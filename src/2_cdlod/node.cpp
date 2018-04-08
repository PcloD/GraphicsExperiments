#include "node.h"
#include "heightmap.h"
#include <camera.h>
#include <algorithm>

namespace dw
{
	Node::Node(HeightMap* heightMap, float node_size, int lod_depth, float x, float z, float height_scale)
	{
		x_pos = x;
		z_pos = z;
		size = node_size;

		if (lod_depth == 1)
		{
			top_left = nullptr;
			top_right = nullptr;
			bottom_left = nullptr;
			bottom_right = nullptr;
			max_height = heightMap->max_height(x_pos, z_pos, size, size) * height_scale;
			min_height = heightMap->min_height(x_pos, z_pos, size, size ) * height_scale;
		}
		else
		{
			float half_size = size / 2.0f;

			top_left = new Node(heightMap, half_size, lod_depth - 1, x_pos, z_pos, height_scale);
			top_right = new Node(heightMap, half_size, lod_depth - 1, x_pos + half_size, z_pos, height_scale);
			bottom_left = new Node(heightMap, half_size, lod_depth - 1, x_pos, z_pos + half_size, height_scale);
			bottom_right = new Node(heightMap, half_size, lod_depth - 1, x_pos + half_size, z_pos + half_size, height_scale);

			max_height = std::max(std::max(top_left->max_height, top_right->max_height),
								  std::max(bottom_left->max_height, bottom_right->max_height));

			min_height = std::max(std::max(top_left->min_height, top_right->min_height),
								  std::max(bottom_left->min_height, bottom_right->min_height));
		}
	}

	Node::~Node()
	{
		if (top_left)
		{
			delete top_left;
			top_left = nullptr;
		}

		if (top_right)
		{
			delete top_right;
			top_right = nullptr;
		}

		if (bottom_left)
		{
			delete bottom_left;
			bottom_left = nullptr;
		}

		if (bottom_right)
		{
			delete bottom_right;
			bottom_right = nullptr;
		}
	}

	bool Node::lod_select(std::vector<float>& ranges, int lod_level, Camera *camera, std::vector<Node*>& sdraw_stack)
	{
		current_range = ranges[lod_level];

		if (!in_sphere(current_range, camera->m_position))
			return false;

		if (!in_frustum(camera))
			return true;

		if (lod_level == 0)
		{
			full_resolution = true;
			sdraw_stack.push_back(this);
			return true;
		}
		else
		{
			if (!in_sphere(ranges[lod_level - 1], camera->m_position))
			{
				full_resolution = true;
				sdraw_stack.push_back(this);
			}
			else
			{
				Node *child;
				child = top_left;

				if (!child->lod_select(ranges, lod_level - 1, camera, sdraw_stack)) 
				{
					child->full_resolution = false;
					child->current_range = current_range;
					sdraw_stack.push_back(child);
				}

				child = top_right;
				if (!child->lod_select(ranges, lod_level - 1, camera, sdraw_stack)) 
				{
					child->full_resolution = false;
					child->current_range = current_range;
					sdraw_stack.push_back(child);
				}

				child = bottom_left;
				if (!child->lod_select(ranges, lod_level - 1, camera, sdraw_stack)) 
				{
					child->full_resolution = false;
					child->current_range = current_range;
					sdraw_stack.push_back(child);
				}

				child = bottom_right;
				if (!child->lod_select(ranges, lod_level - 1, camera, sdraw_stack)) 
				{
					child->full_resolution = false;
					child->current_range = current_range;
					sdraw_stack.push_back(child);
				}
			}

			return true;
		}
	}

	inline float squared(float v) { return v * v; }

	bool Node::in_sphere(float r, glm::vec3 s)
	{
		float dist_squared = r * r;
		glm::vec3 c1 = glm::vec3(x_pos, min_height, z_pos);
		glm::vec3 c2 = glm::vec3(x_pos + size, max_height, z_pos + size);
		
		if (s.x < c1.x) dist_squared -= squared(s.x - c1.x);
		else if (s.x > c2.x) dist_squared -= squared(s.x - c2.x);
		if (s.y < c1.y) dist_squared -= squared(s.y - c1.y);
		else if (s.y > c2.y) dist_squared -= squared(s.y - c2.y);
		if (s.z < c1.z) dist_squared -= squared(s.z - c1.z);
		else if (s.z > c2.z) dist_squared -= squared(s.z - c2.z);

		return dist_squared > 0;
	}

	bool Node::in_frustum(Camera *camera)
	{
		dw::AABB aabb;
		aabb.min = glm::vec3(x_pos, min_height, z_pos);
		aabb.max = glm::vec3(x_pos + size, max_height, z_pos + size);
		return dw::intersects(camera->m_frustum, aabb);
	}
}