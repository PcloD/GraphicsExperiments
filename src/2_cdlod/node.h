#pragma once

#include <vector>
#include <glm.hpp>

class HeightMap;
struct Camera;

namespace dw
{
	struct Node;
	
	struct Node
	{
		float max_height;
		float min_height;
		float x_pos;
		float z_pos;
		float size;
		float current_range;
		bool  full_resolution;
		Node* top_left;
		Node* top_right;
		Node* bottom_left;
		Node* bottom_right;

		Node(HeightMap* heightMap, float node_size, int lod_depth, float x, float z, float height_scale);
		~Node();
		bool lod_select(std::vector<float>& ranges, int lod_level, Camera *camera, std::vector<Node*>& sdraw_stack);
		bool in_sphere(float radius, glm::vec3 position);
		bool in_frustum(Camera *camera);
	};
}