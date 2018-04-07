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
#include "terrain_patch.h"
#include "heightmap.h"

#define CAMERA_SPEED 0.01f
#define CAMERA_SENSITIVITY 0.02f
#define CAMERA_ROLL 0.0

using namespace math;


struct DW_ALIGNED(16) TerrainUniforms
{
	glm::mat4 view_proj;
};

class CDLOD : public dw::Application
{
private:
    Camera* m_camera;
	TerrainPatch* m_patch;
	Shader* m_vs;
	Shader* m_fs;
	ShaderProgram* m_program;
	RasterizerState* m_rs;
	DepthStencilState* m_ds;
	UniformBuffer* m_ubo;
	TerrainUniforms m_uniforms;
	HeightMap m_heightmap;
    float m_heading_speed = 0.0f;
    float m_sideways_speed = 0.0f;
    bool m_mouse_look = false;
	
public:
    bool init(int argc, const char* argv[]) override
    {
        m_camera = new Camera(45.0f,
                              0.1f,
                              100000.0f,
                              ((float)m_width)/((float)m_height),
                              glm::vec3(0.0f, 0.0f, 10.0f),
                              glm::vec3(0.0f, 0.0f, -1.0f));
        
		m_patch = new TerrainPatch(32, 32, &m_device);

		load_resources();
		m_heightmap.initialize("heightmap.r16", 1024, 1024, &m_device);

		float height = m_heightmap.max_height(0, 0, 32, 32);
		printf("%f\n", height);

		height = m_heightmap.max_height(32, 32, 32, 32);
		printf("%f\n", height);

		height = m_heightmap.max_height(0, 0, 1024, 1024);
		printf("%f\n", height);

        return true;
    }

	void load_resources()
	{
		std::string vs_str;
		Utility::ReadText("shader/terrain_vs.glsl", vs_str);

		std::string fs_str;
		Utility::ReadText("shader/terrain_fs.glsl", fs_str);

		m_vs = m_device.create_shader(vs_str.c_str(), ShaderType::VERTEX);
		m_fs = m_device.create_shader(fs_str.c_str(), ShaderType::FRAGMENT);

		if (!m_vs || !m_fs)
		{
			LOG_FATAL("Failed to create Shaders");
			return;
		}

		Shader* shaders[] = { m_vs, m_fs };
		m_program = m_device.create_shader_program(shaders, 2);

		RasterizerStateCreateDesc rs_desc;
		DW_ZERO_MEMORY(rs_desc);
		rs_desc.cull_mode = CullMode::NONE;
		rs_desc.fill_mode = FillMode::WIREFRAME;
		rs_desc.front_winding_ccw = true;
		rs_desc.multisample = true;
		rs_desc.scissor = false;

		m_rs = m_device.create_rasterizer_state(rs_desc);

		DepthStencilStateCreateDesc ds_desc;
		DW_ZERO_MEMORY(ds_desc);
		ds_desc.depth_mask = true;
		ds_desc.enable_depth_test = true;
		ds_desc.enable_stencil_test = false;
		ds_desc.depth_cmp_func = ComparisonFunction::LESS_EQUAL;

		m_ds = m_device.create_depth_stencil_state(ds_desc);

		BufferCreateDesc uboDesc;
		DW_ZERO_MEMORY(uboDesc);
		uboDesc.data = nullptr;
		uboDesc.data_type = DataType::FLOAT;
		uboDesc.size = sizeof(TerrainUniforms);
		uboDesc.usage_type = BufferUsageType::DYNAMIC;

		m_ubo = m_device.create_uniform_buffer(uboDesc);
	}

    void update(double delta) override
    {
        updateCamera();

		m_uniforms.view_proj = m_camera->m_view_projection;

		void* ptr = m_device.map_buffer(m_ubo, BufferMapType::WRITE);
		memcpy(ptr, &m_uniforms, sizeof(TerrainUniforms));
		m_device.unmap_buffer(m_ubo);

		m_device.bind_framebuffer(nullptr);
		float clear[] = { 0.3f, 0.3f, 0.3f, 1.0f };
		m_device.clear_framebuffer(ClearTarget::ALL, clear);
		m_device.set_viewport(m_width, m_height, 0, 0);

		m_device.bind_rasterizer_state(m_rs);
		m_device.bind_depth_stencil_state(m_ds);
		m_device.bind_shader_program(m_program);
		m_device.bind_uniform_buffer(m_ubo, ShaderType::VERTEX, 0);

		m_device.bind_vertex_array(m_patch->m_vao);
		m_device.set_primitive_type(PrimitiveType::TRIANGLES);
		m_device.draw_indexed(m_patch->m_index_count);
    }
    
    void shutdown() override
    {
		delete m_patch;
		delete m_camera;
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
