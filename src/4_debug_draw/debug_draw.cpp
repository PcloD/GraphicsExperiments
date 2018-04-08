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

class DebugDrawDemo : public dw::Application
{
private:
    Camera* m_camera;
    Camera* m_debug_camera;
    float m_heading_speed = 0.0f;
    float m_sideways_speed = 0.0f;
    bool m_mouse_look = false;
    dd::Renderer m_debug_renderer;
    glm::vec3 m_min_extents;
    glm::vec3 m_max_extents;
    glm::vec3 m_pos;
    glm::vec3 m_color;
    float m_rotation;
    float m_grid_spacing;
    float m_grid_y;
    bool m_debug_mode = false;
    glm::mat4 m_model;
	dw::AABB m_aabb;
	int m_count = 0;
    
public:
    bool init(int argc, const char* argv[]) override
    {
        m_camera = new Camera(45.0f,
                              0.1f,
                              100.0f,
                              ((float)m_width)/((float)m_height),
                              glm::vec3(0.0f, 0.0f, 10.0f),
                              glm::vec3(0.0f, 0.0f, -1.0f));
        
        m_debug_camera = new Camera(45.0f,
                              0.1f,
                              10000.0f,
                              ((float)m_width)/((float)m_height),
                              glm::vec3(0.0f, 0.0f, 10.0f),
                              glm::vec3(0.0f, 0.0f, -1.0f));
        
        m_min_extents = glm::vec3(-10.0f);
        m_max_extents = glm::vec3(10.0f);
        m_pos = glm::vec3(40.0f);
        m_color = glm::vec3(1.0f, 0.0f, 0.0f);
        m_rotation = 60.0f;
        m_grid_spacing = 1.0f;
        m_grid_y = 0.0f;

		m_aabb.min = glm::vec3(-10.0f, -10.0f, -10.0f);
		m_aabb.max = glm::vec3(10.0f, 10.0f, 10.0f);
        
        return m_debug_renderer.init(&m_device);
    }
    
    void update(double delta) override
    {
		m_count = 0;

        updateCamera();
        
        m_device.bind_framebuffer(nullptr);
        m_device.set_viewport(m_width, m_height, 0, 0);
        
        float clear[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        m_device.clear_framebuffer(ClearTarget::ALL, clear);

		if (dw::intersects(m_camera->m_frustum, m_aabb))
		{
			m_debug_renderer.aabb(m_aabb.min, m_aabb.max, m_color);
			m_count++;
		}
        
        ImGui::Begin("Debug Draw");
        
        ImGui::InputFloat3("Min Extents", &m_min_extents[0]);
        ImGui::InputFloat3("Max Extents", &m_max_extents[0]);
        ImGui::InputFloat3("Position", &m_pos[0]);
        ImGui::ColorEdit3("Color", &m_color[0]);
        ImGui::InputFloat("Rotation", &m_rotation);
        ImGui::InputFloat("Grid Spacing", &m_grid_spacing);
        ImGui::InputFloat("Grid Y-Level", &m_grid_y);
        
        if (ImGui::Button("Toggle Debug Camera"))
        {
            m_debug_mode = !m_debug_mode;
        }

		std::string text = "Culling Passed: ";
		text += std::to_string(m_count);

		ImGui::Text(text.c_str());
        
        ImGui::End();
        
        m_debug_renderer.capsule(20.0f, 5.0f, glm::vec3(-20.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0));
        //m_debug_renderer.grid(101.0f, 101.0f, m_grid_y, m_grid_spacing, glm::vec3(1.0f));
        m_debug_renderer.sphere(5.0f, glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        //m_model = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation), glm::vec3(0.0f, 1.0f, 0.0f));
        //m_debug_renderer.obb(m_min_extents, m_max_extents, m_model, m_color);
        
		if (m_debug_mode)
		{
			m_debug_renderer.aabb(m_aabb.min, m_aabb.max, m_color);
			m_debug_renderer.frustum(m_camera->m_projection, m_camera->m_view, glm::vec3(0.0f, 1.0f, 0.0f));
		}
            
        m_debug_renderer.render(nullptr, m_width, m_height, m_debug_mode ? m_debug_camera->m_view_projection : m_camera->m_view_projection);
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
        
        if (m_debug_mode)
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

DW_DECLARE_MAIN(DebugDrawDemo)
