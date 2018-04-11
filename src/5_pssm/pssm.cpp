#include <iostream>

#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <application.h>
#include <camera.h>
#include <render_device.h>
#include <utility.h>
#include <scene.h>
#include <material.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <stb_image.h>

#include <vec3.h>
#include <vector>

#include <Macros.h>
#include <debug_draw.h>

#define CAMERA_SPEED 0.05f
#define CAMERA_SENSITIVITY 0.02f
#define CAMERA_ROLL 0.0

#define MAX_POINT_LIGHTS 256
#define MAX_SPOT_LIGHT 32

#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

struct DW_ALIGNED(16) DirectionalLight
{
	glm::vec4 color;
	glm::vec4 direction;
};

struct DW_ALIGNED(16) PointLight
{
	glm::vec4 color;
	glm::vec4 position_radius;
};

struct DW_ALIGNED(16) SpotLight
{
	glm::vec4 color;
	glm::vec4 direction;
	glm::vec4 position;
};

struct DW_ALIGNED(16) LightUniforms
{
	glm::vec4 count; // x: dir, y: point, z: spot, w: cascade count
	DirectionalLight dir_light;
	PointLight point_lights[MAX_POINT_LIGHTS];
	SpotLight spot_lights[MAX_SPOT_LIGHT];
};

struct PSSMSettings
{
	int   num_cascades;
	float size;
	float near_plane;
	float distance;
	bool  visualize_cascades;
	bool  debug_mode;
	glm::vec3 direction;
};

class PSSM : public dw::Application
{
private:
    Camera* m_camera;
    Camera* m_debug_camera;
    float m_heading_speed = 0.0f;
    float m_sideways_speed = 0.0f;
    bool m_mouse_look = false;
    dd::Renderer m_debug_renderer;
	PSSMSettings m_settings;
	glm::vec3 m_frustum_corners[8];
	glm::mat4 m_light_view;
    
public:
    bool init(int argc, const char* argv[]) override
    {
        m_camera = new Camera(45.0f,
							  NEAR_PLANE,
							  FAR_PLANE,
                              ((float)m_width)/((float)m_height),
                              glm::vec3(0.0f, 0.0f, 10.0f),
                              glm::vec3(0.0f, 0.0f, -1.0f));
        
        m_debug_camera = new Camera(45.0f,
                              0.1f,
                              10000.0f,
                              ((float)m_width)/((float)m_height),
                              glm::vec3(0.0f, 0.0f, 10.0f),
                              glm::vec3(0.0f, 0.0f, -1.0f));

		m_settings.near_plane = 0.1f;
		m_settings.num_cascades = 3;
		m_settings.size = 1024;
		m_settings.distance = 500.0f;
		m_settings.debug_mode = false;
		m_settings.visualize_cascades = false;
		
		glm::vec3 dir = glm::vec3(1.0f, -1.0f, 0.0f);
		m_settings.direction = glm::normalize(dir);
        
        return m_debug_renderer.init(&m_device);
    }

	void update_frustum_splits()
	{
		glm::mat4 inv_view_proj = glm::inverse(m_camera->m_view_projection);

		for (int i = 0; i < 8; i++)
		{
			glm::vec4 v = inv_view_proj * dd::kFrustumCorners[i];
			v = v / v.w;
			m_frustum_corners[i] = glm::vec3(v.x, v.y, v.z);
		}

		glm::vec3 near_top_middle = (dd::kFrustumCorners[5] + dd::kFrustumCorners[6]) / 2.0f;
		glm::vec3 near_bottom_middle = (dd::kFrustumCorners[4] + dd::kFrustumCorners[7]) / 2.0f;
		glm::vec3 far_top_middle = (dd::kFrustumCorners[1] + dd::kFrustumCorners[2]) / 2.0f;
		glm::vec3 far_bottom_middle = (dd::kFrustumCorners[0] + dd::kFrustumCorners[3]) / 2.0f;
		glm::vec3 near_middle = (near_top_middle + near_bottom_middle) / 2.0f;
		glm::vec3 far_middle = (far_top_middle + far_bottom_middle) / 2.0f;
		glm::vec3 middle = (near_middle + far_middle) / 2.0f;
		glm::vec3 light_pos = middle - m_settings.direction * m_settings.distance;
		glm::vec3 right = glm::normalize(glm::cross(m_settings.direction, glm::vec3(0.0f, 1.0f, 0.0f)));
		glm::vec3 up = glm::normalize(glm::cross(right, m_settings.direction));

		m_light_view = glm::lookAt(light_pos, middle, up);

		glm::vec3 light_view_corners[8];

		glm::vec3 min = glm::vec3(INFINITY, INFINITY, INFINITY);
		glm::vec3 max = glm::vec3(-INFINITY, -INFINITY, -INFINITY);

		for (int i = 0; i < 8; i++)
		{
			glm::vec4 v = m_light_view * glm::vec4(m_frustum_corners[i].x, m_frustum_corners[i].y, m_frustum_corners[i].z, 1.0f);
			light_view_corners[i] = glm::vec3(v.x, v.y, v.z);

			if (light_view_corners[i].x > max.x)
				max.x = light_view_corners[i].x;
			if (light_view_corners[i].y > max.y)
				max.y = light_view_corners[i].y;
			if (light_view_corners[i].z > max.z)
				max.z = light_view_corners[i].z;

			if (light_view_corners[i].x < min.x)
				min.x = light_view_corners[i].x;
			if (light_view_corners[i].y < min.y)
				min.y = light_view_corners[i].y;
			if (light_view_corners[i].z < min.z)
				min.z = light_view_corners[i].z;
		}

		glm::mat4 inv_light_view = glm::inverse(m_light_view);
		glm::vec4 light_min = inv_light_view * glm::vec4(min.x, min.y, min.z, 1.0f);
		glm::vec4 light_max = inv_light_view * glm::vec4(max.x, max.y, max.z, 1.0f);

		m_debug_renderer.line(light_min, glm::vec3(light_max.x, light_min.y, light_min.z), glm::vec3(1.0f, 0.0f, 0.0f));
		m_debug_renderer.line(glm::vec3(light_max.x, light_min.y, light_min.z), 
							  glm::vec3(light_max.x, light_max.y, light_min.z),
							  glm::vec3(1.0f, 0.0f, 0.0f));
		m_debug_renderer.line(glm::vec3(light_max.x, light_max.y, light_min.z),
							  glm::vec3(light_min.x, light_max.y, light_min.z),
							  glm::vec3(1.0f, 0.0f, 0.0f));
		m_debug_renderer.line(glm::vec3(light_min.x, light_max.y, light_min.z),
							  light_min,
							  glm::vec3(1.0f, 0.0f, 0.0f));

		float c_log, c_uniform;


	}
    
    void update(double delta) override
    {
        updateCamera();
		update_frustum_splits();
        
        m_device.bind_framebuffer(nullptr);
        m_device.set_viewport(m_width, m_height, 0, 0);
        
        float clear[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        m_device.clear_framebuffer(ClearTarget::ALL, clear);

		

		if (ImGui::Begin("PSSM"))
		{
			ImGui::Checkbox("Visualize Cascades", &m_settings.visualize_cascades);
			ImGui::Checkbox("Debug Mode", &m_settings.debug_mode);
			ImGui::InputInt("Num Cascades", &m_settings.num_cascades);
			ImGui::InputFloat("Cascade Size", &m_settings.size);
			ImGui::InputFloat("Near Plane", &m_settings.near_plane);
		}
		ImGui::End();

		if (m_settings.debug_mode)
			m_debug_renderer.frustum(m_camera->m_projection, m_camera->m_view, glm::vec3(0.0f, 1.0f, 0.0f));

        m_debug_renderer.render(nullptr, m_width, m_height, m_settings.debug_mode ? m_debug_camera->m_view_projection : m_camera->m_view_projection);
    }
    
    void shutdown() override
    {
        m_debug_renderer.shutdown();
        delete m_debug_camera;
        delete m_camera;
    }
    
    void key_pressed(int code) override
    {
        if(code == GLFW_KEY_W)
            m_heading_speed = CAMERA_SPEED;
        else if(code == GLFW_KEY_S)
            m_heading_speed = -CAMERA_SPEED;
        
        if(code == GLFW_KEY_A)
            m_sideways_speed = -CAMERA_SPEED;
        else if(code == GLFW_KEY_D)
            m_sideways_speed = CAMERA_SPEED;
        
        if(code == GLFW_KEY_E)
            m_mouse_look = true;
    }
    
    void key_released(int code) override
    {
        if(code == GLFW_KEY_W || code == GLFW_KEY_S)
            m_heading_speed = 0.0f;
        
        if(code == GLFW_KEY_A || code == GLFW_KEY_D)
            m_sideways_speed = 0.0f;
        
        if(code == GLFW_KEY_E)
            m_mouse_look = false;
    }
    
    void updateCamera()
    {
        Camera* current = m_camera;
        
        if (m_settings.debug_mode)
            current = m_debug_camera;
        
        float forward_delta = m_heading_speed * m_delta;
        float right_delta = m_sideways_speed * m_delta;
        
        current->set_translation_delta(current->m_forward, forward_delta);
        current->set_translation_delta(current->m_right, right_delta);
        
        if(m_mouse_look)
        {
            // Activate Mouse Look
            current->set_rotatation_delta(glm::vec3((float)(m_mouse_delta_y * CAMERA_SENSITIVITY * m_delta),
                                                     (float)(m_mouse_delta_x * CAMERA_SENSITIVITY * m_delta),
                                                     (float)(CAMERA_ROLL * CAMERA_SENSITIVITY * m_delta)));
        }
        else
        {
            current->set_rotatation_delta(glm::vec3((float)(0),
                                                     (float)(0),
                                                     (float)(0)));
        }
        
        current->update();
    }
};

DW_DECLARE_MAIN(PSSM)
