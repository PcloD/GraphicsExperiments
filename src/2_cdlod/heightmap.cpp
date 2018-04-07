#include "heightmap.h"
#include <render_device.h>
#include <stdio.h>

HeightMap::HeightMap()
{
	
}

HeightMap::~HeightMap()
{

}

bool HeightMap::initialize(std::string file, int width, int height, RenderDevice* device)
{
	m_device = device;

	int error;
	FILE* filePtr;
	unsigned long long imageSize, count;

	filePtr = fopen(file.c_str(), "rb");

	imageSize = width * height;
	m_data = new unsigned short[imageSize];
	if (!m_data)
	{
		return false;
	}

	count = fread(m_data, sizeof(unsigned short), imageSize, filePtr);
	if (count != imageSize)
	{
		return false;
	}

	error = fclose(filePtr);
	if (error != 0)
	{
		return false;
	}

	Texture2DCreateDesc desc;

	desc.data = m_data;
	desc.format = TextureFormat::R16_FLOAT;
	desc.height = height;
	desc.width = width;
	desc.mipmap_levels = 1;

	m_texture = m_device->create_texture_2d(desc);

	return true;
}

void HeightMap::shutdown()
{
	if (m_data)
	{
		delete m_data;
		m_data = nullptr;
	}

	if (m_texture)
		m_device->destroy(m_texture);
}

Texture2D* HeightMap::texture()
{
	return m_texture;
}

float HeightMap::max_height(int _x, int _y, int _width, int _height)
{
	uint16_t max_val = 0;

	for (int y = _y; y < _height; y++)
	{
		for (int x = _x; x < _width; x++)
		{
			if (m_data[_width * y + x] > max_val)
				max_val = m_data[_width * y + x];
		}
	}

	return float(max_val)/float(UINT16_MAX);
}

float HeightMap::min_height(int _x, int _y, int _width, int _height)
{
	uint16_t min_val = 0;

	for (int y = _y; y < _height; y++)
	{
		for (int x = _x; x < _width; x++)
		{
			if (m_data[_width * y + x] < min_val)
				min_val = m_data[_width * y + x];
		}
	}

	return float(min_val)/float(UINT16_MAX);
}