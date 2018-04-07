#include "terrain.h"
#include "heightmap.h"
#include "node.h"
#include "terrain_patch.h"

#include <utility.h>
#include <render_device.h>
#include <logger.h>
#include <camera.h>

namespace dw
{
	Terrain::Terrain(std::string file, int size, int lod_depth, float scale, RenderDevice* device)
	{
		m_device = device;
		m_lod_depth = lod_depth;
		m_leaf_node_size = 1.0f;

		m_full_patch = new TerrainPatch(32, 32, m_device);
		m_half_patch = new TerrainPatch(16, 16, m_device);

		m_ranges.push_back(1);
		for (int i = 1; i < m_lod_depth; i++)
		{
			m_ranges.push_back(m_ranges[i - 1] + pow(2, i + 2));
			std::cout << m_ranges[i - 1] << std::endl;
		}

		m_height_map = new HeightMap();
		m_height_map->initialize(file, size, size, device);

		float rootNodeSize = m_leaf_node_size * pow(2, m_lod_depth - 1);

		int gridWidth = floor(size / rootNodeSize);
		int gridHeight = floor(size / rootNodeSize);

		m_grid.resize(gridWidth);
		for (int i = 0; i < gridWidth; i++) 
		{
			m_grid[i].resize(gridHeight);
			for (int j = 0; j < gridHeight; j++) 
			{
				float xPos = i * rootNodeSize;
				float zPos = j * rootNodeSize;
				m_grid[i][j] = new Node(m_height_map, rootNodeSize, m_lod_depth, xPos, zPos, scale);
			}
		}

		std::string vs_str;
		Utility::ReadText("shader/terrain_vs.glsl", vs_str);

		std::string fs_str;
		Utility::ReadText("shader/terrain_fs.glsl", fs_str);

		m_vs = m_device->create_shader(vs_str.c_str(), ShaderType::VERTEX);
		m_fs = m_device->create_shader(fs_str.c_str(), ShaderType::FRAGMENT);

		if (!m_vs || !m_fs)
		{
			LOG_FATAL("Failed to create Shaders");
			return;
		}

		Shader* shaders[] = { m_vs, m_fs };
		m_program = m_device->create_shader_program(shaders, 2);

		RasterizerStateCreateDesc rs_desc;
		DW_ZERO_MEMORY(rs_desc);
		rs_desc.cull_mode = CullMode::NONE;
		rs_desc.fill_mode = FillMode::WIREFRAME;
		rs_desc.front_winding_ccw = true;
		rs_desc.multisample = true;
		rs_desc.scissor = false;

		m_rs = m_device->create_rasterizer_state(rs_desc);

		DepthStencilStateCreateDesc ds_desc;
		DW_ZERO_MEMORY(ds_desc);
		ds_desc.depth_mask = true;
		ds_desc.enable_depth_test = true;
		ds_desc.enable_stencil_test = false;
		ds_desc.depth_cmp_func = ComparisonFunction::LESS_EQUAL;

		m_ds = m_device->create_depth_stencil_state(ds_desc);

		BufferCreateDesc uboDesc;
		DW_ZERO_MEMORY(uboDesc);
		uboDesc.data = nullptr;
		uboDesc.data_type = DataType::FLOAT;
		uboDesc.size = 256 * 1024;
		uboDesc.usage_type = BufferUsageType::DYNAMIC;

		m_terrain_ubo = m_device->create_uniform_buffer(uboDesc);

		DW_ZERO_MEMORY(uboDesc);
		uboDesc.data = nullptr;
		uboDesc.data_type = DataType::FLOAT;
		uboDesc.size = sizeof(PerFrameUniform);
		uboDesc.usage_type = BufferUsageType::DYNAMIC;

		m_camera_ubo = m_device->create_uniform_buffer(uboDesc);

		//m_root = new Node(m_height_map, size, m_lod_depth - 1, 0, 0, 100.0f);
	}

	Terrain::~Terrain()
	{
		m_device->destroy(m_camera_ubo);
		m_device->destroy(m_terrain_ubo);
		m_device->destroy(m_program);
		m_device->destroy(m_vs);
		m_device->destroy(m_fs);

		for (unsigned int i = 0; i < m_grid.size(); i++) 
		{
			for (unsigned int j = 0; j < m_grid[0].size(); j++) 
			{
				delete m_grid[i][j];
			}
		}

		delete m_height_map;
		delete m_half_patch;
		delete m_full_patch;
	}

	void Terrain::render(Camera* lod_camera, Camera* draw_camera, int width, int height)
	{
		m_patch_list.clear();

		// Select Nodes
		for (unsigned int i = 0; i < m_grid.size(); i++) 
		{
			for (unsigned int j = 0; j < m_grid[0].size(); j++) 
			{
				m_grid[i][j]->lod_select(m_ranges, m_lod_depth - 1, lod_camera, m_patch_list);
			}
		}

		assert(m_patch_list.size() < MAX_PATCHES);

		std::cout << m_patch_list.size() << std::endl;

		// Update Uniforms
		m_per_frame.view = draw_camera->m_view;
		m_per_frame.proj = draw_camera->m_projection;
		m_per_frame.pos = glm::vec4(draw_camera->m_position.x, draw_camera->m_position.y, draw_camera->m_position.z, 0.0f);

		char* ptr = (char*)m_device->map_buffer(m_camera_ubo, BufferMapType::WRITE);
		memcpy(ptr, &m_per_frame, sizeof(PerFrameUniform));
		m_device->unmap_buffer(m_camera_ubo);

		ptr = (char*)m_device->map_buffer(m_terrain_ubo, BufferMapType::WRITE);

		for (int i = 0; i < m_patch_list.size(); i++)
		{
			Node* node = m_patch_list[i];
			char* current_ptr = ptr + 256 * i;

			glm::vec3 translation = glm::vec3(node->x_pos, 0.0f, node->z_pos);
			glm::vec3 grid_dim = node->full_resolution ? glm::vec3(32, 32, 0) : glm::vec3(16, 16, 0);
			float scale = node->size;
			float range = node->current_range;

			m_uniforms[i].translation_range = glm::vec4(translation.x, translation.y, translation.z, range);
			m_uniforms[i].griddim_scale = glm::vec4(grid_dim.x, grid_dim.y, scale, 0.0f);

			memcpy(current_ptr, &m_uniforms[i], sizeof(TerrainUniforms));
		}

		m_device->unmap_buffer(m_terrain_ubo);

		m_device->bind_framebuffer(nullptr);
		m_device->set_viewport(width, height, 0, 0);
		float clear[] = { 0.3f, 0.3f, 0.3f, 1.0f };
		m_device->clear_framebuffer(ClearTarget::ALL, clear);

		m_device->bind_rasterizer_state(m_rs);
		m_device->bind_depth_stencil_state(m_ds);
		m_device->bind_shader_program(m_program);
		m_device->set_primitive_type(PrimitiveType::TRIANGLES);
		m_device->bind_uniform_buffer(m_camera_ubo, ShaderType::VERTEX, 0);

		// Draw
		for (int i = 0; i < m_patch_list.size(); i++)
		{
			Node* node = m_patch_list[i];
			TerrainPatch* patch = m_half_patch;

			if (node->full_resolution)
				patch = m_full_patch;

			m_device->bind_uniform_buffer_range(m_terrain_ubo, ShaderType::VERTEX, 1, 256 * i, sizeof(TerrainUniforms));
			m_device->bind_vertex_array(patch->m_vao);
			m_device->draw_indexed(patch->m_index_count);
		}
	}
}