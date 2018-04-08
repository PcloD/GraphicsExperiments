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

#include <vec2.h>
#include <vec3.h>

#include <macros.h>
#include <debug_draw.h>
#include <debug_draw.h>
#include "terrain.h"

#define CAMERA_SPEED 0.1f
#define CAMERA_SENSITIVITY 0.02f
#define CAMERA_ROLL 0.0
#define FAR_PLANE 10000.0f

using namespace math;

class CDLOD : public dw::Application
{
private:
    Camera* m_camera;
	Camera* m_debug_camera;
	dw::Terrain* m_terrain;
	dd::Renderer m_debug_renderer;
    float m_heading_speed = 0.0f;
    float m_sideways_speed = 0.0f;
    bool m_mouse_look = false;
	bool m_debug_mode = false;
	
public:
    bool init(int argc, const char* argv[]) override
    {
        m_camera = new Camera(45.0f,
                              0.1f,
							  FAR_PLANE,
                              ((float)m_width)/((float)m_height),
                              glm::vec3(5.0f, 5.0f, 5.0f),
                              glm::vec3(0.0f, 0.0f, -1.0f));

		m_debug_camera = new Camera(45.0f,
							  0.1f,
							  10000.0f,
							  ((float)m_width) / ((float)m_height),
							  glm::vec3(5.0f, 5.0f, 5.0f),
							  glm::vec3(0.0f, 0.0f, -1.0f));

		m_terrain = new dw::Terrain("heightmap.r16", 1024, 6, 50.0f, FAR_PLANE, &m_device);

        return m_debug_renderer.init(&m_device);;
    }

    void update(double delta) override
    {
        updateCamera();

		ImGui::Begin("CDLOD");

		if (ImGui::Button("Toggle Debug Camera"))
		{
			m_debug_mode = !m_debug_mode;
		}

		ImGui::End();

		m_device.bind_framebuffer(nullptr);
		m_device.set_viewport(m_width, m_height, 0, 0);

		float clear[] = { 0.3f, 0.3f, 0.3f, 1.0f };
		m_device.clear_framebuffer(ClearTarget::ALL, clear);

		m_terrain->render(m_camera, m_debug_mode ? m_debug_camera : m_camera, m_width, m_height, &m_debug_renderer);

		if (m_debug_mode)
			m_debug_renderer.frustum(m_camera->m_projection, m_camera->m_view, glm::vec3(0.0f, 1.0f, 0.0f));

		m_debug_renderer.render(nullptr, m_width, m_height, m_debug_mode ? m_debug_camera->m_view_projection : m_camera->m_view_projection);
    }
    
    void shutdown() override
    {
		m_debug_renderer.shutdown();
		delete m_debug_camera;
		delete m_terrain;
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

		if (m_debug_mode)
			current = m_debug_camera;

		float forward_delta = m_heading_speed * m_delta;
		float right_delta = m_sideways_speed * m_delta;

		current->set_translation_delta(current->m_forward, forward_delta);
		current->set_translation_delta(current->m_right, right_delta);

		if (m_mouse_look)
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

DW_DECLARE_MAIN(CDLOD)
