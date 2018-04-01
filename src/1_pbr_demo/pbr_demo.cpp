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
#include <imgui_dock.h>
#include <json.hpp>
#include <nfd.h>
#include "project.h"

#define CAMERA_SPEED 0.01f
#define CAMERA_SENSITIVITY 0.02f
#define CAMERA_ROLL 0.0
#define TEXTURE_TYPE "TextureType"
#define VIEWPORT_PADDING 5

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

struct EditorState
{
	bool show_asset_browser;
	bool show_inspector;
	bool show_heirarchy;
	bool show_viewport;
	bool show_material_editor;
};

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
	std::string m_selected_file;
	DirectoryEntry m_root_entry;
	DirectoryEntry* m_selected_dir;
	bool m_dock_changed = true;
	ImVec2 m_last_dock_size;
	ImVec2 m_last_dock_pos;
	Framebuffer* m_offscreen_fbo;
	Texture2D*   m_color_rt;
	Texture2D*   m_depth_rt;
	Project*	 m_current_project;
	EditorState m_editor_state;
	char* m_string_buffer;

protected:
	void print_dir(DirectoryEntry& dir)
	{
		for (int i = 0; i < dir.directories.size(); i++)
		{
			DirectoryEntry* entry = &dir.directories[i];

			ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ((m_selected_dir == entry) ? ImGuiTreeNodeFlags_Selected : 0);
			
			bool open = ImGui::TreeNodeEx(dir.directories[i].name.c_str(), node_flags);

			if (ImGui::IsItemClicked())
				m_selected_dir = entry;

			if (open)
			{
				print_dir(dir.directories[i]);
				ImGui::TreePop();
			}
		}
	}

	void print_files()
	{
		if (ImGui::Button("Import Asset"))
		{

		}
		ImGui::SameLine();
		if (ImGui::Button("New..."))
		{

		}

		if (m_selected_dir)
		{
			for (int i = 0; i < m_selected_dir->files.size(); i++)
			{
				ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ((m_selected_file == m_selected_dir->files[i]) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				ImGui::TreeNodeEx(m_selected_dir->files[i].c_str(), node_flags);

				if (ImGui::IsItemClicked())
					m_selected_file = m_selected_dir->files[i];

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload(TEXTURE_TYPE, m_selected_file.c_str(), m_selected_file.length());

					ImGui::BeginTooltip();
					ImGui::Text(m_selected_file.c_str());
					ImGui::EndTooltip();
					ImGui::EndDragDropSource();
				}
			}
		}
	}

    bool init(int argc, const char* argv[]) override
    {
        m_camera = new Camera(45.0f,
                            0.1f,
                            1000.0f,
                            ((float)m_width)/((float)m_height),
                            glm::vec3(0.0f, 0.0f, 10.0f),
                            glm::vec3(0.0f, 0.0f, -1.0f));
        
		m_renderer = new dw::Renderer(&m_device, m_width, m_height);

		if (argc > 1)
			open_project(argv[1]);

		ImGui::InitDock();

		m_offscreen_fbo = nullptr;
		m_color_rt = nullptr;
		m_depth_rt = nullptr;
		m_scene = nullptr;
		m_selected_dir = nullptr;

		m_editor_state.show_asset_browser = true;
		m_editor_state.show_inspector = true;
		m_editor_state.show_heirarchy = true;
		m_editor_state.show_viewport = true;
		m_editor_state.show_material_editor = true;

        return true;
    }

    void update(double delta) override
    {
		update_camera();
		render_editor_gui();
		if (m_scene)
			m_renderer->render(m_camera, m_last_dock_size.x, m_last_dock_size.y, m_offscreen_fbo);
		m_device.bind_framebuffer(nullptr);
    }

    void shutdown() override
    {
		m_device.destroy(m_offscreen_fbo);
		m_device.destroy(m_color_rt);
		m_device.destroy(m_depth_rt);
		
		delete m_scene;
		delete m_renderer;
		delete m_camera;
    }

	bool open_project(std::string path)
	{
#ifdef WIN32
		std::replace(path.begin(), path.end(), '\\', '/');
#endif
		std::string projectJson;

		if (!Utility::ReadText(path, projectJson))
			return false;

		nlohmann::json json = nlohmann::json::parse(projectJson.c_str());

		m_current_project = new Project();
		std::string name = json["name"];
		m_current_project->name = name;
		std::size_t found = path.find_last_of("/\\");
		m_current_project->project_directory = path.substr(0, found);

		Utility::change_current_working_directory(m_current_project->project_directory);

		auto platforms = json["platforms"];

		for (auto& platform : platforms)
		{
			std::string platformStr = platform;
			m_current_project->platforms.push_back(platformStr);
		}

		find_assets(m_current_project->project_directory + "/assets", m_root_entry);

		return true;
	}

	void close_project()
	{
		if (m_current_project)
		{
			delete m_current_project;
			m_current_project = nullptr;

			m_root_entry.name.clear();
			m_root_entry.full_path.clear();
			m_root_entry.files.clear();
			m_root_entry.directories.clear();
		}
	}

	bool open_scene(std::string path)
	{
		m_scene = dw::Scene::load(path, &m_device, m_renderer);

		if (!m_scene)
			return false;

		m_renderer->set_scene(m_scene);
		return true;
	}

	void close_scene()
	{
		if (m_scene)
		{
			delete m_scene;
			m_scene = nullptr;
			m_renderer->set_scene(nullptr);
		}
	}

	void rebuild_framebuffer()
	{
		if (m_offscreen_fbo)
			m_device.destroy(m_offscreen_fbo);
		if (m_color_rt)
			m_device.destroy(m_color_rt);
		if (m_depth_rt)
			m_device.destroy(m_depth_rt);

		m_camera->update_projection(45.0f, 0.1f, 1000.0f, m_last_dock_size.x / m_last_dock_size.y);

		Texture2DCreateDesc rtDesc;
		DW_ZERO_MEMORY(rtDesc);
		rtDesc.format = TextureFormat::R8G8B8A8_UNORM;
		rtDesc.height = m_last_dock_size.y;
		rtDesc.width = m_last_dock_size.x;
		rtDesc.mipmap_levels = 10;

		m_color_rt = m_device.create_texture_2d(rtDesc);

		if (!m_color_rt)
		{
			std::cout << "Failed to create render target" << std::endl;
		}

		rtDesc.format = TextureFormat::D32_FLOAT_S8_UINT;

		m_depth_rt = m_device.create_texture_2d(rtDesc);

		if (!m_depth_rt)
		{
			std::cout << "Failed to create depth target" << std::endl;
		}

		FramebufferCreateDesc fbDesc;
		DW_ZERO_MEMORY(fbDesc);
		fbDesc.renderTargetCount = 1;

		fbDesc.depthStencilTarget.texture = m_depth_rt;
		fbDesc.depthStencilTarget.arraySlice = 0;
		fbDesc.depthStencilTarget.mipSlice = 0;

		fbDesc.renderTargets[0].texture = m_color_rt;
		fbDesc.renderTargets[0].arraySlice = 0;
		fbDesc.renderTargets[0].mipSlice = 0;

		m_offscreen_fbo = m_device.create_framebuffer(fbDesc);

		if (!m_offscreen_fbo)
		{
			std::cout << "Failed to create Framebuffer" << std::endl;
		}
	}

	void render_editor_gui()
	{
		ImGuizmo::BeginFrame(m_last_dock_pos, m_last_dock_size);

		int dock_flags = ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar;

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(m_width, m_height));
		
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

		if (ImGui::Begin("Editor", (bool*)0, dock_flags))
		{
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Open Project..."))
					{
						nfdresult_t res = NFD_OpenDialog("tproj", nullptr, &m_string_buffer);

						if (res != NFD_CANCEL)
						{
							std::string path = m_string_buffer;
							free(m_string_buffer);

							if (res == NFD_ERROR)
							{
								LOG_ERROR("Failed to open project");
							}
							else if (res == NFD_OKAY)
								open_project(path);
						}
					}
					if (ImGui::MenuItem("Close Project"))
					{
						close_project();
					}
					if (ImGui::MenuItem("Open Scene..."))
					{
						nfdresult_t res = NFD_OpenDialog("json", nullptr, &m_string_buffer);

						if (res != NFD_CANCEL)
						{
							std::string path = m_string_buffer;
							free(m_string_buffer);

							if (res == NFD_ERROR)
							{
								LOG_ERROR("Failed to open project");
							}
							else if (res == NFD_OKAY)
								open_scene(path);
						}
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Edit"))
				{
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("View"))
				{
					ImGui::MenuItem("Asset Browser", NULL, &m_editor_state.show_asset_browser);
					ImGui::MenuItem("Inspector", NULL, &m_editor_state.show_inspector);
					ImGui::MenuItem("Heirarchy", NULL, &m_editor_state.show_heirarchy);
					ImGui::MenuItem("Viewport", NULL, &m_editor_state.show_viewport);
					ImGui::MenuItem("Material Editor", NULL, &m_editor_state.show_material_editor);
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			ImGui::Button("Translate");
			ImGui::SameLine();
			ImGui::Button("Rotate");
			ImGui::SameLine();
			ImGui::Button("Scale");

			// dock layout by hard-coded or .ini file
			ImGui::BeginDockspace();

			ImGui::SetNextDock("Editor", ImGuiDockSlot_Bottom);
			if (ImGui::BeginDock("Asset Browser", &m_editor_state.show_asset_browser))
			{
				ImGui::Columns(2);
				ImGui::SetColumnWidth(0, 200.0f);
				print_dir(m_root_entry);
				ImGui::NextColumn();
				print_files();
			}
			ImGui::EndDock();

			ImGui::SetNextDock("Editor", ImGuiDockSlot_Top);
			if (ImGui::BeginDock("Viewport", &m_editor_state.show_viewport))
			{
				ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
				ImVec2 frame_padding = ImGui::GetStyle().FramePadding;
				ImVec2 current = ImGui::GetWindowSize();
				current.x -= (window_padding.x + frame_padding.x);
				current.y -= (window_padding.y + frame_padding.y + VIEWPORT_PADDING);
				//std::cout << "current:" << current.x << ", " << current.y << std::endl;
				if (ImGui::IsMouseDragging())
				{
					if (current.x != m_last_dock_size.x || current.y != m_last_dock_size.y)
						m_dock_changed = true;
				}
				else
				{
					if (m_dock_changed)
					{
						m_dock_changed = false;
						m_last_dock_size = current;
						rebuild_framebuffer();
					}
					else if (!ImGui::IsAnyMouseDown() && (current.x != m_last_dock_size.x || current.y != m_last_dock_size.y))
					{
						// @TODO: Fix framebuffer recreation upon moving when viewport dock is floating.
						m_last_dock_size = current;
						rebuild_framebuffer();
						std::cout << "m_last_dock_size:" << m_last_dock_size.x << ", " << m_last_dock_size.y << std::endl;
					}
				}

				ImGuizmo::SetDrawlist();
				m_last_dock_size = current;
				m_last_dock_pos = ImGui::GetWindowPos();
				m_last_dock_pos.x += window_padding.x;

				if (m_color_rt)
					dw::imageWithTexture(m_color_rt, m_last_dock_size);
			}
			ImGui::EndDock();

			ImGui::SetNextDock("Editor", ImGuiDockSlot_Tab);
			if (ImGui::BeginDock("Material Editor", &m_editor_state.show_material_editor)) {
				ImGui::Text("I'm BentleyBlanks!");
			}
			ImGui::EndDock();

			ImGui::SetNextDock("Editor", ImGuiDockSlot_Left);
			if (ImGui::BeginDock("Inspector", &m_editor_state.show_inspector)) {
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

						if (ImGui::BeginDragDropTarget())
						{
							if (ImGui::AcceptDragDropPayload(TEXTURE_TYPE))
								entity.m_override_mat->m_albedo_tex = dw::Material::load_texture(m_selected_file, &m_device, true);
							ImGui::EndDragDropTarget();
						}

						ImGui::Text("Normal");
						dw::imageWithTexture(entity.m_override_mat->texture_normal(), ImVec2(100, 100));

						if (ImGui::BeginDragDropTarget())
						{
							if (ImGui::AcceptDragDropPayload(TEXTURE_TYPE))
								entity.m_override_mat->m_normal_tex = dw::Material::load_texture(m_selected_file, &m_device, true);
							ImGui::EndDragDropTarget();
						}

						ImGui::Text("Metalness");
						dw::imageWithTexture(entity.m_override_mat->texture_metalness(), ImVec2(100, 100));

						if (ImGui::BeginDragDropTarget())
						{
							if (ImGui::AcceptDragDropPayload(TEXTURE_TYPE))
								entity.m_override_mat->m_metalness_tex = dw::Material::load_texture(m_selected_file, &m_device, true);
							ImGui::EndDragDropTarget();
						}

						ImGui::Text("Roughness");
						dw::imageWithTexture(entity.m_override_mat->texture_roughness(), ImVec2(100, 100));

						if (ImGui::BeginDragDropTarget())
						{
							if (ImGui::AcceptDragDropPayload(TEXTURE_TYPE))
								entity.m_override_mat->m_roughness_tex = dw::Material::load_texture(m_selected_file, &m_device, true);
							ImGui::EndDragDropTarget();
						}
					}
				}
			}
			ImGui::EndDock();

			ImGui::SetNextDock("Editor", ImGuiDockSlot_Right);
			if (ImGui::BeginDock("Heirarchy", &m_editor_state.show_heirarchy)) {
				if (m_scene)
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
				}
			}
			ImGui::EndDock();

			ImGui::EndDockspace();
		}
		ImGui::End();

		ImGui::PopStyleVar();
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

		ImGuizmo::SetRect(m_last_dock_pos.x, m_last_dock_pos.y, m_last_dock_size.x, m_last_dock_size.y);
	    ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, useSnap ? &snap[0] : NULL);
	    ImGuizmo::DecomposeMatrixToComponents(matrix, position, rotation, scale);
	}
};

DW_DECLARE_MAIN(Demo)
