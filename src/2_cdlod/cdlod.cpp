#include <iostream>

#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <application.h>
#include <camera.h>
#include <render_device.h>
#include <utility.h>
#include <scene.h>
#include <material.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <stb_image.h>

#include <vec2.h>
#include <vec3.h>

#include <macros.h>
#include <debug_draw.h>

#define CAMERA_SPEED 0.1f
#define CAMERA_SENSITIVITY 0.02f
#define CAMERA_ROLL 0.0

using namespace math;

void drawAABB(dd::Renderer* renderer, const vec3f& min, const vec3f& max, const glm::mat4& view_proj, const int& width, const int& height)
{
	glm::vec3 _min = glm::vec3(min.x, min.y, min.z);
	glm::vec3 _max = glm::vec3(max.x, max.y, max.z);
	glm::vec3 _pos = glm::vec3(0.0f);

	renderer->aabb(_min, _max, _pos, glm::vec3(0.0f, 1.0f, 0.0f));
}

struct Node
{
	vec3f min_extents;
	vec3f max_extents;

	Node* children[4];

	Node(vec3f min, vec3f max, int level, int max_level)
	{
		for (int i = 0; i < 4; i++)
			children[i] = nullptr;

		min_extents = min;
		max_extents = max;

		std::cout << "Level : " << level << "/" << max_level << std::endl;
		min.print();
		max.print();
		std::cout << std::endl;

		if (level < max_level)
		{
			vec3f half_size = (max_extents - min_extents) / 2.0f;
			level++;

			for (int y = 0; y < 2; y++)
			{
				for (int x = 0; x < 2; x++)
				{
					vec3f min = min_extents;
					min.z = min.z + half_size.z * (1.0f - y);
					min.x = min.x + half_size.x * x;

					vec3f max = max_extents;
					max.z = max.z - half_size.z * y;
					max.x = max.x - half_size.x * (1.0f - x);

					children[2 * y + x] = new Node(min, max, level, max_level);
				}
			}
		}
	}

	~Node()
	{
		for (int i = 0; i < 4; i++)
		{
			if (children[i])
			{
				delete children[i];
				children[i] = nullptr;
			}
		}
	}

	void render(dd::Renderer* renderer, int level, int current_level, const glm::mat4& view_proj, const int& width, const int& height)
	{
		if (current_level != level)
		{
			current_level++;

			for (int i = 0; i < 4; i++)
			{
				if (children[i])
					children[i]->render(renderer, level, current_level, view_proj, width, height);
			}
		}
		else
		{
			drawAABB(renderer, min_extents, max_extents, view_proj, width, height);
		}
	}
};

struct TerrainVertex
{
    glm::vec2 pos;
};

struct DW_ALIGNED(16) TerrainUniforms
{
    glm::mat4 view_proj;
    glm::vec4 rect;
    glm::vec4 scale;
};

struct Terrain
{
    VertexArray* m_vao;
    IndexBuffer* m_ibo;
    VertexBuffer* m_vbo;
    InputLayout* m_il;
    Shader* m_vs;
    Shader* m_fs;
    ShaderProgram* m_program;
    UniformBuffer* m_ubo;
    RenderDevice* m_device;
    RasterizerState* m_rs;
    DepthStencilState* m_ds;
    TerrainVertex* m_vertices;
    TerrainUniforms m_uniforms;
    std::vector<uint32_t> m_indices;
    Texture2D* m_height_map;
	SamplerState* m_sampler;

    Terrain(float x, float z, float distance, RenderDevice* device)
    {
        m_device = device;
        
        float x_half = x/2.0f * distance;
        float z_half = z/2.0f * distance;
        
        int idx = 0;
        m_vertices = new TerrainVertex[(x + 1) * (z + 1)];
        
        for (int z = -z_half; z <= z_half; z += distance)
        {
            for (int x = -x_half; x <= x_half; x += distance)
            {
                m_vertices[idx].pos = glm::vec2(x, z);
                idx++;
            }
        }
        
        // 0 1 2
        // 3 4 5
        // 6 7 8
        
        for (int i = 0; i < z; i++)
        {
            for (int j = 0; j < x; j++)
            {
                m_indices.push_back((x + 1) * i + j);
                m_indices.push_back((x + 1) * (i + 1) + j);
                m_indices.push_back((x + 1) * i + (j + 1));
                
                m_indices.push_back((x + 1) * i + j + 1);
                m_indices.push_back((x + 1) * (i + 1) + j);
                m_indices.push_back((x + 1) * (i + 1) + (j + 1));
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
        
        BufferCreateDesc bc;
        InputLayoutCreateDesc ilcd;
        VertexArrayCreateDesc vcd;
        
        DW_ZERO_MEMORY(bc);
        bc.data = &m_vertices[0];
        bc.data_type = DataType::FLOAT;
        bc.size = sizeof(TerrainVertex) * (x + 1) * (z + 1);
        bc.usage_type = BufferUsageType::STATIC;
        
        m_vbo = m_device->create_vertex_buffer(bc);
        
        DW_ZERO_MEMORY(bc);
        bc.data = &m_indices[0];
        bc.data_type = DataType::UINT32;
        bc.size = sizeof(uint32_t) * m_indices.size();
        bc.usage_type = BufferUsageType::STATIC;
        
        m_ibo = m_device->create_index_buffer(bc);
        
        InputElement elements[] =
        {
            { 2, DataType::FLOAT, false, 0, "POSITION" }
        };
        
        DW_ZERO_MEMORY(ilcd);
        ilcd.elements = elements;
        ilcd.num_elements = 1;
        ilcd.vertex_size = sizeof(float) * 2;
        
        m_il = m_device->create_input_layout(ilcd);
        
        DW_ZERO_MEMORY(vcd);
        vcd.index_buffer = m_ibo;
        vcd.vertex_buffer = m_vbo;
        vcd.layout = m_il;
        
        m_vao = m_device->create_vertex_array(vcd);
        
        if (!m_vao || !m_ibo || !m_vbo)
        {
            LOG_FATAL("Failed to create Vertex Buffers/Arrays");
            return;
        }

		SamplerStateCreateDesc ssDesc;
		DW_ZERO_MEMORY(ssDesc);

		ssDesc.max_anisotropy = 0;
		ssDesc.min_filter = TextureFilteringMode::LINEAR;
		ssDesc.mag_filter = TextureFilteringMode::LINEAR;
		ssDesc.wrap_mode_u = TextureWrapMode::CLAMP_TO_EDGE;
		ssDesc.wrap_mode_v = TextureWrapMode::CLAMP_TO_EDGE;
		ssDesc.wrap_mode_w = TextureWrapMode::CLAMP_TO_EDGE;

		m_sampler = m_device->create_sampler_state(ssDesc);
        
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
        uboDesc.size = sizeof(TerrainUniforms);
        uboDesc.usage_type = BufferUsageType::DYNAMIC;
        
        m_ubo = m_device->create_uniform_buffer(uboDesc);
        
        m_uniforms.rect.x = x + 1;
        m_uniforms.rect.y = z + 1;
        m_uniforms.rect.z = x_half;
        m_uniforms.rect.w = z_half;
        m_uniforms.scale.x = 1.0f;
        
        load(1024, 1024);
    }
    
    void render(glm::mat4 view_proj, uint32_t width, uint32_t height)
    {
        ImGui::SliderFloat("Terrain Scale", &m_uniforms.scale.x, 1.0f, 300.0f);
		ImGui::Image((ImTextureID)m_height_map->id, ImVec2(1025, 1025));
        
        m_uniforms.view_proj = view_proj;
        
        void* ptr = m_device->map_buffer(m_ubo, BufferMapType::WRITE);
        memcpy(ptr, &m_uniforms, sizeof(TerrainUniforms));
        m_device->unmap_buffer(m_ubo);
        
        m_device->bind_rasterizer_state(m_rs);
        m_device->bind_depth_stencil_state(m_ds);
        m_device->bind_framebuffer(nullptr);
        m_device->set_viewport(width, height, 0, 0);
        m_device->bind_shader_program(m_program);
		m_device->bind_sampler_state(m_sampler, ShaderType::VERTEX, 0);
        m_device->bind_uniform_buffer(m_ubo, ShaderType::VERTEX, 0);
        m_device->bind_texture(m_height_map, ShaderType::VERTEX, 0);
        m_device->bind_vertex_array(m_vao);
        m_device->set_primitive_type(PrimitiveType::TRIANGLES);
        m_device->draw_indexed(m_indices.size());
    }
    
    void shutdown()
    {
		m_device->destroy(m_sampler);
        m_device->destroy(m_ubo);
        m_device->destroy(m_program);
        m_device->destroy(m_vs);
        m_device->destroy(m_fs);
        m_device->destroy(m_ibo);
        m_device->destroy(m_vbo);
        m_device->destroy(m_vao);
        m_device->destroy(m_ds);
        m_device->destroy(m_rs);
    }
    
    bool load(int width, int height)
    {
        int error;
        FILE* filePtr;
        unsigned long long imageSize, count;
        unsigned short* rawImage;
        
        // Open the 16 bit raw height map file for reading in binary.
        filePtr = fopen("heightmap.r16", "rb");
        
        // Calculate the size of the raw image data.
        imageSize = width * height;
        
        // Allocate memory for the raw image data.
        rawImage = new unsigned short[imageSize];
        if(!rawImage)
        {
            return false;
        }
        
        // Read in the raw image data.
        count = fread(rawImage, sizeof(unsigned short), imageSize, filePtr);
        if(count != imageSize)
        {
            return false;
        }

		//for (int i = 0; i < 100; i++)
		//{
		//	uint16_t data = rawImage[i];
		//	float fdata = (float)data;
		//	std::cout << data << std::endl;
		//	std::cout << fdata << std::endl;
		//}
        
        // Close the file.
        error = fclose(filePtr);
        if(error != 0)
        {
            return false;
        }
        
        Texture2DCreateDesc desc;
        
        desc.data = rawImage;
        desc.format = TextureFormat::R16_FLOAT;
        desc.height = height;
        desc.width = width;
        desc.mipmap_levels = 1;
        
        m_height_map = m_device->create_texture_2d(desc);

        // Release the bitmap image data.
        delete [] rawImage;
        rawImage = 0;
        
        return true;
    }
};

class CDLOD : public dw::Application
{
private:
    Camera* m_camera;
    float m_heading_speed = 0.0f;
    float m_sideways_speed = 0.0f;
    bool m_mouse_look = false;
	Node* m_root;
	int m_level = 0;
	dd::Renderer m_debug_renderer;
	Terrain* m_terrain;

public:
    bool init(int argc, const char* argv[]) override
    {
        m_camera = new Camera(45.0f,
                              0.1f,
                              100000.0f,
                              ((float)m_width)/((float)m_height),
                              glm::vec3(0.0f, 0.0f, 10.0f),
                              glm::vec3(0.0f, 0.0f, -1.0f));
        
		m_root = new Node(vec3f(-512.0f, -10.0f, -512.0f), vec3f(512.0f, 10.0f, 512.0f), 0, 4);

		m_debug_renderer.init(&m_device);
		m_terrain = new Terrain(1024, 1024, 1, &m_device);

        return true;
    }
    
    void update(double delta) override
    {
        updateCamera();
        
        m_device.bind_framebuffer(nullptr);
        m_device.set_viewport(m_width, m_height, 0, 0);
        
        float clear[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        m_device.clear_framebuffer(ClearTarget::ALL, clear);
        
		ImGui::Begin("Quad Tree");

		ImGui::InputInt("Level", &m_level);

		ImGui::End();
        
		m_root->render(&m_debug_renderer, m_level, 0, m_camera->m_view_projection, m_width, m_height);
		m_debug_renderer.render(nullptr, m_width, m_height, m_camera->m_view_projection);
		m_terrain->render(m_camera->m_view_projection, m_width, m_height);
    }
    
    void shutdown() override
    {
		m_terrain->shutdown();
        delete m_terrain;
		
		m_debug_renderer.shutdown();

		delete m_camera;
		delete m_root;
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
    
    void updateCamera()
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
};

DW_DECLARE_MAIN(CDLOD)
