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

using namespace math;


struct SSVertex
{
	bool visible;
	vec2f v;
};

void drawAABB(const vec3f& min, const vec3f& max, const glm::mat4& view_proj, const int& width, const int& height)
{
	SSVertex verts[8];

	vec3f size = max - min;
	int i = 0;

	for (float y = 0; y < 2; y++)
	{
		for (float x = 0; x < 2; x++)
		{
			for (float z = 0; z < 2; z++)
			{
				vec3f v0 = vec3f(min.x + size.x * x, min.y + size.y * y, min.z + size.z * z);

				glm::vec4 v = view_proj * glm::vec4(v0.x, v0.y, v0.z, 1.0f);
				v /= v.w;

				if ((v.x <= 1.0f) && (v.x >= -1.0f) && (v.y <= 1.0f) && (v.y >= -1.0f) && (v.z <= 1.0f) && (v.z >= -1.0f))
					verts[i].visible = true;
				else
					verts[i].visible = false;

				float vx = (v.x + 1.0f) / 2.0f;
				vx *= width;
				float vy = 1.0f - ((v.y + 1.0f) / 2.0f);
				vy *= height;

				verts[i].v = vec2f(vx, vy);

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

struct Node
{
	vec3f min_extents;
	vec3f max_extents;

	Node* children[4];

	Node(vec3f min, vec3f max, int level, int max_level)
	{
		for (int i = 0; i < 4; i++)
			children[i] = nullptr;

		min_extents = min;
		max_extents = max;

		std::cout << "Level : " << level << "/" << max_level << std::endl;
		min.print();
		max.print();
		std::cout << std::endl;

		if (level < max_level)
		{
			vec3f half_size = (max_extents - min_extents) / 2.0f;
			level++;

			for (int y = 0; y < 2; y++)
			{
				for (int x = 0; x < 2; x++)
				{
					vec3f min = min_extents;
					min.z = min.z + half_size.z * (1.0f - y);
					min.x = min.x + half_size.x * x;

					vec3f max = max_extents;
					max.z = max.z - half_size.z * y;
					max.x = max.x - half_size.x * (1.0f - x);

					children[2 * y + x] = new Node(min, max, level, max_level);
				}
			}
		}
	}

	~Node()
	{
		for (int i = 0; i < 4; i++)
		{
			if (children[i])
			{
				delete children[i];
				children[i] = nullptr;
			}
		}
	}

	void render(int level, int current_level, const glm::mat4& view_proj, const int& width, const int& height)
	{
		if (current_level != level)
		{
			current_level++;

			for (int i = 0; i < 4; i++)
			{
				if (children[i])
					children[i]->render(level, current_level, view_proj, width, height);
			}
		}
		else
		{
			drawAABB(min_extents, max_extents, view_proj, width, height);
		}
	}
};

class CDLOD : public dw::Application
{
private:
    Camera* m_camera;
    float m_heading_speed = 0.0f;
    float m_sideways_speed = 0.0f;
    bool m_mouse_look = false;
	Node* m_root;
	int m_level = 0;

public:
    bool init() override
    {
        m_camera = new Camera(45.0f,
                              0.1f,
                              100000.0f,
                              ((float)m_width)/((float)m_height),
                              glm::vec3(0.0f, 0.0f, 10.0f),
                              glm::vec3(0.0f, 0.0f, -1.0f));
        
		m_root = new Node(vec3f(-128.0f, -10.0f, -128.0f), vec3f(128.0f, 10.0f, 128.0f), 0, 4);

        return true;
    }
    
    void update(double delta) override
    {
        updateCamera();
        
        m_device.bind_framebuffer(nullptr);
        m_device.set_viewport(m_width, m_height, 0, 0);
        
        float clear[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        m_device.clear_framebuffer(ClearTarget::ALL, clear);
        
        ImGuiIO& io = ImGui::GetIO();

		ImGui::Begin("Quad Tree");

		ImGui::InputInt("Level", &m_level);

		ImGui::End();
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin("testing", nullptr, ImVec2(io.DisplaySize.x, io.DisplaySize.y), 0.0f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
        
		//drawAABB(vec3f(-4.0f, -1.0f, -1.0f), vec3f(4.0f, 1.0f, 1.0f), m_camera->m_view_projection, m_width, m_height);
		m_root->render(m_level, 0, m_camera->m_view_projection, m_width, m_height);
   
        ImGui::End();
        ImGui::PopStyleVar();
        
        ImGui::ShowDemoWindow();
    }
    
    void shutdown() override
    {
		delete m_camera;
		delete m_root;
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
