#include "terrain_patch.h"
#include <Macros.h>
#include <render_device.h>
#include <glm.hpp>
#include <vector>
#include <stdio.h>

TerrainPatch::TerrainPatch(int width, int height, RenderDevice* device)
{
	std::vector<glm::vec3> vertices;
	std::vector<GLuint>	   index;

	for (int y = 0; y <= height; ++y)
	{
		for (int x = 0; x <= width; ++x)
		{
			float xx = (x - width / 2);
			float yy = (y - height / 2);
			xx = x;
			yy = y;
			vertices.push_back(glm::vec3(xx / width, 0.0, yy / height));
		}
	}

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			index.push_back((x + 0) + (width + 1)*(y + 0));
			index.push_back((x + 1) + (width + 1)*(y + 0));
			index.push_back((x + 1) + (width + 1)*(y + 1));

			index.push_back((x + 1) + (width + 1)*(y + 1));
			index.push_back((x + 0) + (width + 1)*(y + 1));
			index.push_back((x + 0) + (width + 1)*(y + 0));
		}
	}

	BufferCreateDesc desc;
	DW_ZERO_MEMORY(desc);

	desc.data = &vertices[0];
	desc.data_type = DataType::FLOAT;
	desc.size = sizeof(glm::vec3) * vertices.size();
	desc.usage_type = BufferUsageType::STATIC;

	m_vbo = m_device->create_vertex_buffer(desc);

	DW_ZERO_MEMORY(desc);

	desc.data = &index[0];
	desc.data_type = DataType::UINT32;
	desc.size = sizeof(GLuint) * index.size();
	desc.usage_type = BufferUsageType::STATIC;

	m_ibo = m_device->create_index_buffer(desc);

	InputLayoutCreateDesc il_desc;

	InputElement elements[] =
	{
		{ 3, DataType::FLOAT, false, 0, "POSITION" }
	};

	il_desc.num_elements = 1;
	il_desc.vertex_size = sizeof(glm::vec3);
	il_desc.elements = elements;

	m_il = m_device->create_input_layout(il_desc);

	VertexArrayCreateDesc vao_desc;
	DW_ZERO_MEMORY(vao_desc);

	vao_desc.index_buffer = m_ibo;
	vao_desc.vertex_buffer = m_vbo;
	vao_desc.layout = m_il;

	m_vao = m_device->create_vertex_array(vao_desc);

	m_index_count = index.size();
}

TerrainPatch::~TerrainPatch()
{
	delete m_il;
	m_device->destroy(m_vao);
	m_device->destroy(m_vbo);
	m_device->destroy(m_ibo);
}