#pragma once

struct VertexArray;
struct VertexBuffer;
struct IndexBuffer;
struct InputLayout;
class RenderDevice;

struct TerrainPatch
{
	VertexArray*  m_vao;
	VertexBuffer* m_vbo;
	IndexBuffer*  m_ibo;
	InputLayout*  m_il;
	RenderDevice* m_device;
	int			  m_index_count;

	TerrainPatch(int width, int height, RenderDevice* device);
	~TerrainPatch();
};