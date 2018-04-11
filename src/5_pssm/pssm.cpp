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
#include "shadows.h"

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

class PSSM : public dw::Application
{
private:
    Camera* m_camera;
    Camera* m_debug_camera;
    float m_heading_speed = 0.0f;
    float m_sideways_speed = 0.0f;
    bool m_mouse_look = false;
    dd::Renderer m_debug_renderer;
	ShadowSettings m_shadow_settings;
	Shadows m_shadows;
	bool  visualize_cascades;
	bool  show_shadow_frustum;
	bool  show_frustum_splits;
	bool  debug_mode;
	glm::vec3 direction;
	glm::mat4 test_proj;
	glm::mat4 test_view;

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

		debug_mode = false;
		visualize_cascades = false;
		show_shadow_frustum = false;
		show_frustum_splits = false;
		glm::vec3 dir = glm::vec3(1.0f, -1.0f, 0.0f);
		direction = glm::normalize(dir);

		m_shadow_settings.lambda = 0.75f;
		m_shadow_settings.split_count = 3;

		m_shadows.initialize(m_shadow_settings, m_camera, m_width, m_height, direction);
		test_view = glm::lookAt(glm::vec3(0.0f), glm::vec3(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		test_proj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
	
        return m_debug_renderer.init(&m_device);
    }
    
    void update(double delta) override
    {
        updateCamera();
		m_shadows.update(m_camera, direction);

		for (int i = 0; i < m_shadow_settings.split_count; i++)
		{
			FrustumSplit& split = m_shadows.frustum_splits()[i];
			
			if (show_frustum_splits)
			{
				m_debug_renderer.line(split.corners[0], split.corners[3], glm::vec3(1.0f));
				m_debug_renderer.line(split.corners[3], split.corners[2], glm::vec3(1.0f));
				m_debug_renderer.line(split.corners[2], split.corners[1], glm::vec3(1.0f));
				m_debug_renderer.line(split.corners[1], split.corners[0], glm::vec3(1.0f));

				m_debug_renderer.line(split.corners[4], split.corners[7], glm::vec3(1.0f));
				m_debug_renderer.line(split.corners[7], split.corners[6], glm::vec3(1.0f));
				m_debug_renderer.line(split.corners[6], split.corners[5], glm::vec3(1.0f));
				m_debug_renderer.line(split.corners[5], split.corners[4], glm::vec3(1.0f));

				m_debug_renderer.line(split.corners[0], split.corners[4], glm::vec3(1.0f));
				m_debug_renderer.line(split.corners[1], split.corners[5], glm::vec3(1.0f));
				m_debug_renderer.line(split.corners[2], split.corners[6], glm::vec3(1.0f));
				m_debug_renderer.line(split.corners[3], split.corners[7], glm::vec3(1.0f));
			}
			
			if (show_shadow_frustum)
				m_debug_renderer.frustum(m_shadows.split_view_proj(i), glm::vec3(1.0f, 0.0f, 0.0f));
		}

        m_device.bind_framebuffer(nullptr);
        m_device.set_viewport(m_width, m_height, 0, 0);
        
        float clear[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        m_device.clear_framebuffer(ClearTarget::ALL, clear);

		if (ImGui::Begin("PSSM"))
		{
			ImGui::Checkbox("Visualize Cascades", &visualize_cascades);
			ImGui::Checkbox("Show Shadow Frustums", &show_shadow_frustum);
			ImGui::Checkbox("Show Frustum Splits", &show_frustum_splits);
			ImGui::Checkbox("Debug Camera", &debug_mode);
			ImGui::InputInt("Num Cascades", &m_shadow_settings.split_count);
			ImGui::InputFloat("Lambda", &m_shadow_settings.lambda);
			ImGui::DragFloat3("Direction", &direction.x, 0.1f);

			if (ImGui::Button("Reset"))
			{
				m_shadows.initialize(m_shadow_settings, m_camera, m_width, m_height, direction);
			}
		}
		ImGui::End();

		if (debug_mode)
			m_debug_renderer.frustum(m_camera->m_projection, m_camera->m_view, glm::vec3(0.0f, 1.0f, 0.0f));

        m_debug_renderer.render(nullptr, m_width, m_height, debug_mode ? m_debug_camera->m_view_projection : m_camera->m_view_projection);
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
        
        if (debug_mode)
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
