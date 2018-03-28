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
#include <windows.h>

#include <ImGuizmo.h>
#include <imgui_helpers.h>

#define CAMERA_SPEED 0.01f
#define CAMERA_SENSITIVITY 0.02f
#define CAMERA_ROLL 0.0

const char* kMeshAssets[] = 
{
	"mesh/cerberus.tsm",
	"mesh/solid_shader_ball.tsm"
};

const char* kMaterialAssets[] =
{
	"material/mat_bamboo_wood.json",
	"material/mat_copper_rock.json",
	"material/mat_gold_scuffed.json",
	"material/mat_marble_speckled.json",
	"material/mat_oak_floor.json",
	"material/mat_octostone.json",
	"material/mat_redbricks.json",
	"material/mat_rusted_metal.json",
	"material/mat_slate.json",
	"material/mat_untitled_1.json"
};

struct DirectoryEntry
{
	std::string name;
	std::string full_path;
	std::vector<std::string> files;
	std::vector<DirectoryEntry> directories;
};

void find_assets(const std::string& name, DirectoryEntry& root_entry)
{
	std::string pattern(name);
	pattern.append("\\*");
	WIN32_FIND_DATA data;
	HANDLE hFind;
	if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {
		do {
			std::string current = data.cFileName;
			std::string path = name;
			path += "/";
			path += data.cFileName;
			DWORD dwAttr = GetFileAttributes(path.c_str());

			if (dwAttr != 0xffffffff && (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
			{
				if (current != "." && current != "..")
				{
					DirectoryEntry dir;
					dir.name = data.cFileName;
					dir.full_path = path;

					find_assets(path, dir);
					root_entry.directories.push_back(dir);
				}
			}
			else
			{
				if (current != "." && current != "..")
				{
					root_entry.files.push_back(data.cFileName);
				}
			}

		} while (FindNextFile(hFind, &data) != 0);
		FindClose(hFind);
	}
}

class Demo : public dw::Application
{
private:
    float m_heading_speed = 0.0f;
    float m_sideways_speed = 0.0f;
    bool m_mouse_look = false;
	bool m_show_scene_window = false;
    Camera* m_camera;
	ID m_selected_entity = USHRT_MAX;
	dw::Scene* m_scene;
	dw::Renderer* m_renderer;
	char m_name_buffer[128];
	DirectoryEntry m_root_entry;

protected:
	void print_dir(DirectoryEntry& dir, std::string tab)
	{
		for (int i = 0; i < dir.directories.size(); i++)
		{
			std::cout << tab << dir.directories[i].name << std::endl;
			print_dir(dir.directories[i], tab + "\t");
		}

		for (int i = 0; i < dir.files.size(); i++)
		{
			std::cout << tab << dir.files[i] << std::endl;
		}
	}

    bool init() override
    {
        m_camera = new Camera(45.0f,
                            0.1f,
                            1000.0f,
                            ((float)m_width)/((float)m_height),
                            glm::vec3(0.0f, 0.0f, 10.0f),
                            glm::vec3(0.0f, 0.0f, -1.0f));
        
		m_renderer = new dw::Renderer(&m_device, m_width, m_height);
		m_scene = dw::Scene::load("scene.json", &m_device, m_renderer);

		if (!m_scene)
		{
			LOG_ERROR("Failed to load scene!");
			return false;
		}

		m_renderer->set_scene(m_scene);

		find_assets("C:/Users/Dihara/Desktop/TextureExport", m_root_entry);

		print_dir(m_root_entry, "");

        return true;
    }

    void update(double delta) override
    {
		update_camera();
		render_editor_gui();
		ImGui::ShowDemoWindow();
		m_renderer->render(m_camera);
    }

    void shutdown() override
    {
		delete m_scene;
		delete m_renderer;
		delete m_camera;
    }

	void render_editor_gui()
	{
		ImGuizmo::BeginFrame();

		if (m_show_scene_window)
		{
			if (ImGui::Begin("Scene"))
			{
				dw::Entity* entities = m_scene->entities();

				for (int i = 0; i < m_scene->entity_count(); i++)
				{
					if (ImGui::Selectable(entities[i].m_name.c_str(), m_selected_entity == entities[i].id))
					{
						m_selected_entity = entities[i].id;
					}
				}

				if (ImGui::Button("New"))
				{
					dw::Entity e;
					e.m_position = glm::vec3(0.0f, 0.0f, 0.0f);
					e.m_rotation = glm::vec3(0.0f, 0.0f, 0.0f);
					e.m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
					e.m_mesh = nullptr;
					e.m_override_mat = nullptr;
					e.m_transform = glm::mat4(1.0f);
					e.m_program = nullptr;
					e.m_name = "Empty";

					m_scene->add_entity(e);
				}

				ImGui::SameLine();

				if (m_selected_entity != USHRT_MAX)
				{
					if (ImGui::Button("Remove"))
					{
						m_scene->destroy_entity(m_selected_entity);
						m_selected_entity = USHRT_MAX;
					}
				}

				ImGui::End();
			}

			if (ImGui::Begin("Inspector"))
			{
				if (m_selected_entity != USHRT_MAX)
				{
					dw::Entity& entity = m_scene->lookup(m_selected_entity);
					strcpy(&m_name_buffer[0], entity.m_name.c_str());
					ImGui::InputText("Name", &m_name_buffer[0], 128);
					entity.m_name = m_name_buffer;

					ImGui::Separator();

					edit_transform((float*)&m_camera->m_view, (float*)&m_camera->m_projection, &entity.m_position.x, &entity.m_rotation.x, &entity.m_scale.x, (float*)&entity.m_transform);

					ImGui::Separator();

					ImGui::Text("Material");

					if (entity.m_override_mat)
					{
						ImGui::Text("Albedo");
						dw::imageWithTexture(entity.m_override_mat->texture_albedo(), ImVec2(100, 100));
						ImGui::Text("Normal");
						dw::imageWithTexture(entity.m_override_mat->texture_normal(), ImVec2(100, 100));
						ImGui::Text("Metalness");
						dw::imageWithTexture(entity.m_override_mat->texture_metalness(), ImVec2(100, 100));
						ImGui::Text("Roughness");
						dw::imageWithTexture(entity.m_override_mat->texture_roughness(), ImVec2(100, 100));
					}
				}

				ImGui::End();
			}
		}
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

		if (code == GLFW_KEY_H)
			m_show_scene_window = !m_show_scene_window;
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

	void edit_transform(const float* cameraView, float* cameraProjection, float* position, float* rotation, float* scale, float* matrix)
	{
	    static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
	    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
	    static bool useSnap = false;
	    static float snap[3] = { 1.f, 1.f, 1.f };

	    //if (ImGui::IsKeyPressed(90))
	    //    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	    //if (ImGui::IsKeyPressed(69))
	    //    mCurrentGizmoOperation = ImGuizmo::ROTATE;
	    //if (ImGui::IsKeyPressed(82)) // r Key
	    //    mCurrentGizmoOperation = ImGuizmo::SCALE;
	    if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
	        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	    ImGui::SameLine();
	    if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
	        mCurrentGizmoOperation = ImGuizmo::ROTATE;
	    ImGui::SameLine();
	    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
	        mCurrentGizmoOperation = ImGuizmo::SCALE;
	    
	    ImGui::InputFloat3("Tr", position, 3);
	    ImGui::InputFloat3("Rt", rotation, 3);
	    ImGui::InputFloat3("Sc", scale, 3);
	    ImGuizmo::RecomposeMatrixFromComponents(position, rotation, scale, matrix);
	    
	    if (mCurrentGizmoOperation != ImGuizmo::SCALE)
	    {
	        if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
	            mCurrentGizmoMode = ImGuizmo::LOCAL;
	        ImGui::SameLine();
	        if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
	            mCurrentGizmoMode = ImGuizmo::WORLD;
	    }
	    //if (ImGui::IsKeyPressed(83))
	    //    useSnap = !useSnap;
	    ImGui::Checkbox("", &useSnap);
	    ImGui::SameLine();
	    
	    switch (mCurrentGizmoOperation)
	    {
	        case ImGuizmo::TRANSLATE:
	            ImGui::InputFloat3("Snap", &snap[0]);
	            break;
	        case ImGuizmo::ROTATE:
	            ImGui::InputFloat("Angle Snap", &snap[0]);
	            break;
	        case ImGuizmo::SCALE:
	            ImGui::InputFloat("Scale Snap", &snap[0]);
	            break;
	    }
	    ImGuiIO& io = ImGui::GetIO();
	    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	    ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, useSnap ? &snap[0] : NULL);
	    
	    ImGuizmo::DecomposeMatrixToComponents(matrix, position, rotation, scale);
	}
};

DW_DECLARE_MAIN(Demo)
