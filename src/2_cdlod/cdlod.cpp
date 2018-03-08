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

#include <macros.h>

#define CAMERA_SPEED 0.05f
#define CAMERA_SENSITIVITY 0.02f
#define CAMERA_ROLL 0.0

#define DRAW_LINE(list, vertices, v0, v1, color)				  \
if (vertices[v0].visible || vertices[v1].visible)				  \
{																  \
	list->PathLineTo(ImVec2(vertices[v0].v.x, vertices[v0].v.y)); \
	list->PathLineTo(ImVec2(vertices[v1].v.x, vertices[v1].v.y)); \
	list->PathStroke(color, false);								  \
}

struct SSVertex
{
	bool visible;
	glm::vec2 v;
};

class CDLOD : public dw::Application
{
private:
    Camera* m_camera;
    float m_heading_speed = 0.0f;
    float m_sideways_speed = 0.0f;
    bool m_mouse_look = false;

public:
    bool init() override
    {
        m_camera = new Camera(45.0f,
                              0.1f,
                              1000.0f,
                              ((float)m_width)/((float)m_height),
                              glm::vec3(0.0f, 0.0f, 10.0f),
                              glm::vec3(0.0f, 0.0f, -1.0f));
        
        return true;
    }

	void drawAABB(const glm::vec3& min, const glm::vec3& max)
	{
		SSVertex verts[8];

		glm::vec3 size = max - min;
		int i = 0;

		for (float y = min.y; y <= max.y; y += size.y)
		{
			for (float x = min.x; x <= max.x; x += size.x)
			{
				for (float z = min.z; z <= max.z; z += size.y)
				{
					glm::vec3 v0 = glm::vec3(x, y, z);

					glm::vec4 v = m_camera->m_view_projection * glm::vec4(v0.x, v0.y, v0.z, 1.0f);
					v /= v.w;

					if ((v.x <= 1.0f) && (v.x >= -1.0f) && (v.y <= 1.0f) && (v.y >= -1.0f) && (v.z <= 1.0f) && (v.z >= -1.0f))
						verts[i].visible = true;
					else
						verts[i].visible = false;

					float x = (v.x + 1.0f) / 2.0f;
					x *= m_width;
					float y = 1.0f - ((v.y + 1.0f) / 2.0f);
					y *= m_height;

					verts[i].v = glm::vec2(x, y);

					i++;
				}
			}
		}

		ImDrawList* list = ImGui::GetWindowDrawList();

		// 0-1, 1-3, 3-2, 2-0, 0-4, 4-6, 6-2, 1-5, 5-7, 7-3, 4-5, 6-7 
		DRAW_LINE(list, verts, 0, 1, IM_COL32(0, 255, 0, 255));
		DRAW_LINE(list, verts, 1, 3, IM_COL32(0, 255, 0, 255));
		DRAW_LINE(list, verts, 3, 2, IM_COL32(0, 255, 0, 255));
		DRAW_LINE(list, verts, 2, 0, IM_COL32(0, 255, 0, 255));
		DRAW_LINE(list, verts, 0, 4, IM_COL32(0, 255, 0, 255));
		DRAW_LINE(list, verts, 4, 6, IM_COL32(0, 255, 0, 255));
		DRAW_LINE(list, verts, 6, 2, IM_COL32(0, 255, 0, 255));
		DRAW_LINE(list, verts, 1, 5, IM_COL32(0, 255, 0, 255));
		DRAW_LINE(list, verts, 5, 7, IM_COL32(0, 255, 0, 255));
		DRAW_LINE(list, verts, 7, 3, IM_COL32(0, 255, 0, 255));
		DRAW_LINE(list, verts, 4, 5, IM_COL32(0, 255, 0, 255));
		DRAW_LINE(list, verts, 6, 7, IM_COL32(0, 255, 0, 255));
	}
    
    void update(double delta) override
    {
        updateCamera();
        
        m_device.bind_framebuffer(nullptr);
        m_device.set_viewport(m_width, m_height, 0, 0);
        
        float clear[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        m_device.clear_framebuffer(ClearTarget::ALL, clear);
        
        ImGuiIO& io = ImGui::GetIO();
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin("testing", nullptr, ImVec2(io.DisplaySize.x, io.DisplaySize.y), 0.0f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
        
		drawAABB(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
   
        ImGui::End();
        ImGui::PopStyleVar();
        
        ImGui::ShowDemoWindow();
    }
    
    void shutdown() override
    {
        
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
        m_camera->set_translation_delta(m_camera->m_forward, m_heading_speed * m_delta);
        m_camera->set_translation_delta(m_camera->m_right, m_sideways_speed * m_delta);
        
        if(m_mouse_look)
        {
            // Activate Mouse Look
            m_camera->set_rotatation_delta(glm::vec3((float)(m_mouse_delta_y * CAMERA_SENSITIVITY * m_delta),
                                                     (float)(m_mouse_delta_x * CAMERA_SENSITIVITY * m_delta),
                                                     (float)(CAMERA_ROLL * CAMERA_SENSITIVITY * m_delta)));
        }
        else
        {
            m_camera->set_rotatation_delta(glm::vec3((float)(0),
                                                     (float)(0),
                                                     (float)(0)));
        }
        
        m_camera->update();
    }
};

DW_DECLARE_MAIN(CDLOD)
