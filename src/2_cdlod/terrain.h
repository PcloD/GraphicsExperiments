#pragma once

#include <string>
#include <vector>
#include <glm.hpp>
#include <Macros.h>

class Camera;
class HeightMap;
class RenderDevice;
struct TerrainPatch;
struct Shader;
struct ShaderProgram;
struct RasterizerState;
struct DepthStencilState;
struct UniformBuffer;

#define MAX_PATCHES 1024

namespace dw
{
	struct Node;

	struct DW_ALIGNED(16) TerrainUniforms
	{
		glm::vec4 translation_range;
		glm::vec4 griddim_scale;
	};

	struct DW_ALIGNED(16) PerFrameUniform
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 pos;
	};

	class Terrain
	{
	private:
		HeightMap * m_height_map;
		RenderDevice* m_device;
		Node* m_root;
		Shader* m_vs;
		Shader* m_fs;
		ShaderProgram* m_program;
		RasterizerState* m_rs;
		DepthStencilState* m_ds;
		UniformBuffer* m_terrain_ubo;
		UniformBuffer* m_camera_ubo;
		TerrainUniforms m_uniforms[MAX_PATCHES];
		PerFrameUniform m_per_frame;
		TerrainPatch* m_full_patch;
		TerrainPatch* m_half_patch;
		int m_lod_depth;
		int  m_leaf_node_size;
		std::vector<float> m_ranges;
		std::vector<Node*> m_patch_list;
		std::vector< std::vector<Node*> > m_grid;

	public:
		Terrain(std::string file, int size, int lod_depth, float scale, RenderDevice* device);
		~Terrain();
		void render(Camera* camera, int width, int height);
	};
}