#include <iostream>
#include <fstream>

#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <application.h>
#include <camera.h>
#include <render_device.h>
#include <utility.h>
#include <scene.h>
#include <material.h>
#include <macros.h>
#include <renderer.h>
#include <memory>
#include <shadows.h>
#include <debug_draw.h>

#define CAMERA_ROLL 0.0

class GraphicsDemo : public dw::Application
{
private:
    float m_heading_speed = 0.0f;
    float m_sideways_speed = 0.0f;
    bool m_mouse_look = false;
	bool m_show_debug_window = false;
	float m_camera_sensitivity = 0.02f;
	float m_camera_speed = 0.01f;
    Camera* m_camera;
	Camera* m_debug_camera;
	dw::Scene* m_scene;
	dw::Renderer* m_renderer;
	dd::Renderer m_debug_renderer;
	ShadowSettings m_shadow_settings;
	Shadows* m_shadows;
	glm::vec3 direction;
	bool  visualize_cascades;
	bool  show_shadow_frustum;
	bool  show_frustum_splits;
	bool  debug_mode;

protected:
	void debug_window()
	{
		ImGui::Begin("Debug");

		ImGui::DragFloat3("Light Direction", &m_renderer->per_scene_uniform()->directionalLight.direction.x);
		ImGui::ColorPicker3("Light Color", &m_renderer->per_scene_uniform()->directionalLight.color.x);
		ImGui::InputFloat("Camera Sensitivity", &m_camera_sensitivity);
		ImGui::InputFloat("Camera Speed", &m_camera_speed);

		ImGui::End();
	}
	
    bool init(int argc, const char* argv[]) override
    {
        m_camera = new Camera(45.0f,
                            0.1f,
                            1000.0f,
                            ((float)m_width)/((float)m_height),
                            glm::vec3(0.0f, 0.0f, 10.0f),
                            glm::vec3(0.0f, 0.0f, -1.0f));

		m_debug_camera = new Camera(45.0f,
			0.1f,
			10000.0f,
			((float)m_width) / ((float)m_height),
			glm::vec3(0.0f, 0.0f, 10.0f),
			glm::vec3(0.0f, 0.0f, -1.0f));

		debug_mode = false;
		visualize_cascades = false;
		show_shadow_frustum = false;
		show_frustum_splits = false;
		glm::vec3 dir = glm::vec3(1.0f, -1.0f, 0.0f);
		direction = glm::normalize(dir);
        
		m_renderer = new dw::Renderer(&m_device, m_width, m_height);

		m_scene = dw::Scene::load("scene.json", &m_device, m_renderer);

		if (!m_scene)
			return false;

		m_renderer->set_scene(m_scene);

		m_shadow_settings.lambda = 0.75f;
		m_shadow_settings.near_offset = 100.0f;
		m_shadow_settings.split_count = 3;
		m_shadow_settings.shadow_map_size = 1024;

		m_shadows = new Shadows();
		m_shadows->initialize(&m_device, m_shadow_settings, m_camera, m_width, m_height, direction);
		
		return m_debug_renderer.init(&m_device);
    }

	void render_shadow_debug()
	{
		m_device.bind_framebuffer(nullptr);
		m_device.set_viewport(m_width, m_height, 0, 0);

		float clear[] = { 0.3f, 0.3f, 0.3f, 1.0f };
		m_device.clear_framebuffer(ClearTarget::ALL, clear);

		for (int i = 0; i < m_shadow_settings.split_count; i++)
		{
			FrustumSplit& split = m_shadows->frustum_splits()[i];

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
				m_debug_renderer.frustum(m_shadows->split_view_proj(i), glm::vec3(1.0f, 0.0f, 0.0f));
		}

		if (ImGui::Begin("PSSM"))
		{
			ImGui::Checkbox("Visualize Cascades", &visualize_cascades);
			ImGui::Checkbox("Show Shadow Frustums", &show_shadow_frustum);
			ImGui::Checkbox("Show Frustum Splits", &show_frustum_splits);
			ImGui::Checkbox("Debug Camera", &debug_mode);
			ImGui::Checkbox("Stable Shadows", &m_shadows->m_stable_pssm);
			ImGui::InputInt("Num Cascades", &m_shadow_settings.split_count);
			ImGui::InputInt("Shadow Map Size", &m_shadow_settings.shadow_map_size);
			ImGui::InputFloat("Lambda", &m_shadow_settings.lambda);
			ImGui::DragFloat3("Direction", &direction.x, 0.1f);

			if (ImGui::Button("Reset"))
			{
				m_shadows->initialize(&m_device, m_shadow_settings, m_camera, m_width, m_height, direction);
			}
		}
		ImGui::End();

		if (debug_mode)
			m_debug_renderer.frustum(m_camera->m_projection, m_camera->m_view, glm::vec3(0.0f, 1.0f, 0.0f));

		m_debug_renderer.render(nullptr, m_width, m_height, debug_mode ? m_debug_camera->m_view_projection : m_camera->m_view_projection);
	}

    void update(double delta) override
    {
		if (m_show_debug_window)
			debug_window();

		update_camera();

		m_shadows->update(m_camera, direction);
		//m_renderer->render(debug_mode ? m_debug_camera : m_camera, m_width, m_height, nullptr);

		render_shadow_debug();

		m_device.bind_framebuffer(nullptr);
    }

    void shutdown() override
    {
		delete m_shadows;
		m_debug_renderer.shutdown();
		delete m_scene;
		delete m_renderer;
		delete m_debug_camera;
		delete m_camera;
    }

    void key_pressed(int code) override
    {
        if(code == GLFW_KEY_W)
            m_heading_speed = m_camera_speed;
        else if(code == GLFW_KEY_S)
            m_heading_speed = -m_camera_speed;
        
        if(code == GLFW_KEY_A)
            m_sideways_speed = -m_camera_speed;
        else if(code == GLFW_KEY_D)
            m_sideways_speed = m_camera_speed;
        
        if(code == GLFW_KEY_E)
            m_mouse_look = true;

		if (code == GLFW_KEY_H)
			m_show_debug_window = !m_show_debug_window;
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
    
    void update_camera()
    {
		Camera* current = m_camera;

		if (debug_mode)
			current = m_debug_camera;

		float forward_delta = m_heading_speed * m_delta;
		float right_delta = m_sideways_speed * m_delta;

		current->set_translation_delta(current->m_forward, forward_delta);
		current->set_translation_delta(current->m_right, right_delta);

		if (m_mouse_look)
		{
			// Activate Mouse Look
			current->set_rotatation_delta(glm::vec3((float)(m_mouse_delta_y * m_camera_sensitivity * m_delta),
				(float)(m_mouse_delta_x * m_camera_sensitivity * m_delta),
				(float)(CAMERA_ROLL * m_camera_sensitivity * m_delta)));
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

DW_DECLARE_MAIN(GraphicsDemo)
