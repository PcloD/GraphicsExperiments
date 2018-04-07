#pragma once

#include <string>
#include <stdint.h>

struct Texture2D;
class RenderDevice;

class HeightMap
{
public:
	HeightMap();
	~HeightMap();
	bool initialize(std::string file, int width, int height, RenderDevice* device);
	void shutdown();
	Texture2D* texture();
	float max_height(int x, int y, int width, int height);
	float min_height(int x, int y, int width, int height);
	inline int width() { return m_width; };
	inline int height() { return m_height; };

private:
	RenderDevice* m_device;
	Texture2D* m_texture;
	uint16_t* m_data;
	int m_width;
	int m_height;
};