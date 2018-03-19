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

#include <macros.h>

#define CUBEMAP_WIDTH 512
#define CUBEMAP_HEIGHT 512

#define PREFILTER_MIPMAPS 5

#define PREFILTER_WIDTH 128
#define PREFILTER_HEIGHT 128

#define CAMERA_SPEED 0.05f
#define CAMERA_SENSITIVITY 0.02f
#define CAMERA_ROLL 0.0

#define BOX_WIDTH 1.5f
#define BOX_HEIGHT 0.5f
#define BOX_DEPTH 0.5f

#define MAX_POINT_LIGHTS 8
#define VEC3_TO_VEC4(vector) glm::vec4(vector.x, vector.y, vector.z, 1.0f)

#define NUM_ROWS 7
#define NUM_COLUMNS 7

struct DW_ALIGNED(16) PerFrameUniforms
{
    glm::mat4 u_last_vp_mat;
    glm::mat4 u_vp_mat;
    glm::mat4 u_inv_vp_mat;
    glm::mat4 u_proj_mat;
    glm::mat4 u_view_mat;
    glm::vec4 u_view_pos;
    glm::vec4 u_view_dir;
};

struct DW_ALIGNED(16) PerEntityUniforms
{
    glm::mat4 u_mvp_mat;
    glm::mat4 u_model_mat;
    glm::vec4 u_pos;
	glm::vec4 u_metalRough;
};

struct PointLight
{
	glm::vec4 position;
	glm::vec4 color;
};

struct DirectionalLight
{
	glm::vec4 direction;
	glm::vec4 color;
};

struct DW_ALIGNED(16) PerSceneUniforms
{
	PointLight 		 pointLights[MAX_POINT_LIGHTS];
	DirectionalLight directionalLight;
	int				 pointLightCount;
};

struct DW_ALIGNED(16) CubeMapUniforms
{
	glm::mat4 proj;
	glm::mat4 view;
};

const float clear_color[] = { 0.1f, 0.1f, 0.1f, 1.0f};

const float vertices[] = {
    -BOX_WIDTH, -BOX_HEIGHT, -BOX_DEPTH,  0.0f,  0.0f, -1.0f,
    BOX_WIDTH, -BOX_HEIGHT, -BOX_DEPTH,  0.0f,  0.0f, -1.0f,
    BOX_WIDTH,  BOX_HEIGHT, -BOX_DEPTH,  0.0f,  0.0f, -1.0f,
    BOX_WIDTH,  BOX_HEIGHT, -BOX_DEPTH,  0.0f,  0.0f, -1.0f,
    -BOX_WIDTH,  BOX_HEIGHT, -BOX_DEPTH,  0.0f,  0.0f, -1.0f,
    -BOX_WIDTH, -BOX_HEIGHT, -BOX_DEPTH,  0.0f,  0.0f, -1.0f,
    
    -BOX_WIDTH, -BOX_HEIGHT,  BOX_DEPTH,  0.0f,  0.0f,  1.0f,
    BOX_WIDTH, -BOX_HEIGHT,  BOX_DEPTH,  0.0f,  0.0f,  1.0f,
    BOX_WIDTH,  BOX_HEIGHT,  BOX_DEPTH,  0.0f,  0.0f,  1.0f,
    BOX_WIDTH,  BOX_HEIGHT,  BOX_DEPTH,  0.0f,  0.0f,  1.0f,
    -BOX_WIDTH,  BOX_HEIGHT,  BOX_DEPTH,  0.0f,  0.0f,  1.0f,
    -BOX_WIDTH, -BOX_HEIGHT,  BOX_DEPTH,  0.0f,  0.0f,  1.0f,
    
    -BOX_WIDTH,  BOX_HEIGHT,  BOX_DEPTH, -1.0f,  0.0f,  0.0f,
    -BOX_WIDTH,  BOX_HEIGHT, -BOX_DEPTH, -1.0f,  0.0f,  0.0f,
    -BOX_WIDTH, -BOX_HEIGHT, -BOX_DEPTH, -1.0f,  0.0f,  0.0f,
    -BOX_WIDTH, -BOX_HEIGHT, -BOX_DEPTH, -1.0f,  0.0f,  0.0f,
    -BOX_WIDTH, -BOX_HEIGHT,  BOX_DEPTH, -1.0f,  0.0f,  0.0f,
    -BOX_WIDTH,  BOX_HEIGHT,  BOX_DEPTH, -1.0f,  0.0f,  0.0f,
    
    BOX_WIDTH,  BOX_HEIGHT,  BOX_DEPTH,  1.0f,  0.0f,  0.0f,
    BOX_WIDTH,  BOX_HEIGHT, -BOX_DEPTH,  1.0f,  0.0f,  0.0f,
    BOX_WIDTH, -BOX_HEIGHT, -BOX_DEPTH,  1.0f,  0.0f,  0.0f,
    BOX_WIDTH, -BOX_HEIGHT, -BOX_DEPTH,  1.0f,  0.0f,  0.0f,
    BOX_WIDTH, -BOX_HEIGHT,  BOX_DEPTH,  1.0f,  0.0f,  0.0f,
    BOX_WIDTH,  BOX_HEIGHT,  BOX_DEPTH,  1.0f,  0.0f,  0.0f,
    
    -BOX_WIDTH, -BOX_HEIGHT, -BOX_DEPTH,  0.0f, -1.0f,  0.0f,
    BOX_WIDTH, -BOX_HEIGHT, -BOX_DEPTH,  0.0f, -1.0f,  0.0f,
    BOX_WIDTH, -BOX_HEIGHT,  BOX_DEPTH,  0.0f, -1.0f,  0.0f,
    BOX_WIDTH, -BOX_HEIGHT,  BOX_DEPTH,  0.0f, -1.0f,  0.0f,
    -BOX_WIDTH, -BOX_HEIGHT,  BOX_DEPTH,  0.0f, -1.0f,  0.0f,
    -BOX_WIDTH, -BOX_HEIGHT, -BOX_DEPTH,  0.0f, -1.0f,  0.0f,
    
    -BOX_WIDTH,  BOX_HEIGHT, -BOX_DEPTH,  0.0f,  1.0f,  0.0f,
    BOX_WIDTH,  BOX_HEIGHT, -BOX_DEPTH,  0.0f,  1.0f,  0.0f,
    BOX_WIDTH,  BOX_HEIGHT,  BOX_DEPTH,  0.0f,  1.0f,  0.0f,
    BOX_WIDTH,  BOX_HEIGHT,  BOX_DEPTH,  0.0f,  1.0f,  0.0f,
    -BOX_WIDTH,  BOX_HEIGHT,  BOX_DEPTH,  0.0f,  1.0f,  0.0f,
    -BOX_WIDTH,  BOX_HEIGHT, -BOX_DEPTH,  0.0f,  1.0f,  0.0f
};

const float kQuadVertices[] = {
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
    1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
    1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
};

class Demo : public dw::Application
{
private:
    float m_heading_speed = 0.0f;
    float m_sideways_speed = 0.0f;
    bool m_mouse_look = false;
    PerFrameUniforms m_per_frame;
    PerEntityUniforms m_per_entity;
    PerSceneUniforms m_per_scene;
    glm::vec3 m_cube_position = glm::vec3(0.0f);
    glm::vec3 m_cube_scale = glm::vec3(1.0f);
    glm::vec3 m_cube_rotation = glm::vec3(0.0f);
    glm::mat4 m_cube_transform = glm::mat4(1.0f);
    Camera* m_camera;
    InputLayout* m_quadLayout;
    Shader* m_vs;
    Shader* m_fs;
    ShaderProgram* m_program;
    UniformBuffer* m_per_frame_ubo;
    UniformBuffer* m_per_entity_ubo;
    UniformBuffer* m_per_scene_ubo;
    RasterizerState* m_rs;
    DepthStencilState* m_ds;
    Texture2D* m_textureRT;
    Texture2D* m_textureDS;
    Framebuffer* m_offscreenFBO;
    VertexBuffer* m_quadVBO;
    VertexArray* m_quadVAO;
    SamplerState* m_sampler;
	SamplerState* m_cubemapSampler;

    Shader* m_quadVs;
    Shader* m_quadFs;
    ShaderProgram* m_quadProgram;
	//dw::Scene m_Scene;
	//dw::Mesh* m_Mesh;
	//dw::Material* m_Mat;

	uint32_t m_sphereIndexCount;
	IndexBuffer* m_sphereIBO;
	VertexBuffer* m_sphereVBO;
	VertexArray* m_sphereVAO;
	InputLayout* m_sphereIL;
	uint32_t m_uboAlign;

	uint32_t m_cubeIndexCount;
	VertexBuffer* m_cubeVBO;
	VertexArray* m_cubeVAO;
	InputLayout* m_cubeIL;

	Shader* m_latlongToCubeVs;
	Shader* m_latlongToCubeFs;
	ShaderProgram* m_latlongToCubeProgram;

	Shader* m_cubeMapVs;
	Shader* m_cubeMapFs;
	ShaderProgram* m_cubeMapProgram;

	Shader* m_irradianceFs;
	ShaderProgram* m_irradianceProgram;

	Shader* m_prefilterFs;
	ShaderProgram* m_prefilterProgram;

	Shader* m_brdfIntegrateFs;
	ShaderProgram* m_brdfIntegrateProgram;

	Texture2D* m_brdfLUT;
	Framebuffer* m_brdfLUTFBO;

	Texture2D* m_latLongMap;
	TextureCube* m_envMap;
	Framebuffer* m_envMapFBOs[6];
	UniformBuffer* m_cubeMapUBO;

	TextureCube* m_irradianceMap;
	Framebuffer* m_irradianceMapFBOs[6];

	TextureCube* m_prefilteredMap;
	Framebuffer* m_prefilteredMapFBOs[PREFILTER_MIPMAPS][6];

	CubeMapUniforms m_cubemapUniforms[6];
	PerEntityUniforms m_prefilterUniforms[PREFILTER_MIPMAPS];

	PerEntityUniforms m_sphereUniforms[NUM_ROWS * NUM_COLUMNS];
    
protected:
	void saveBRDF()
	{
		int width, height = 0;

		m_device.texture_extents(m_brdfLUT, 0, width, height);

		float* data = new float[width * height * 2];
		float* rgbData = new float[width * height * 3];

		m_device.texture_data(m_brdfLUT, 0, TextureType::TEXTURE2D, data);

		int j = 0;

		for (int i = 0; i < width * height * 2; i += 2)
		{
			rgbData[j++] = data[i];
			rgbData[j++] = data[i + 1];
			rgbData[j++] = 0.0f;
		}

		stbi_write_hdr("prefilter/preintegrate_brdf.hdr", width, height, 3, rgbData);

		delete[]data;
		delete[]rgbData;
	}

	void saveEnvMap()
	{
		int width, height = 0;
		m_device.texture_extents(m_envMap, 0, width, height);

		float* data = new float[width * height * 3];
	
		for (int i = 0; i < 6; i++)
		{
			m_device.texture_data(m_envMap, 0, TextureType::TEXTURECUBE_POSITIVE_X + i, data);
			std::string fileName = "prefilter/env_map_" + std::to_string(i) + ".hdr";
			stbi_write_hdr(fileName.c_str(), width, height, 3, data);
		}

		delete[]data;
	}

	void savePrefilterMap()
	{
		for (int mip = 0; mip < PREFILTER_MIPMAPS; mip++)
		{
			int width, height = 0;
			m_device.texture_extents(m_prefilteredMap, mip, width, height);

			float* data = new float[width * height * 3];

			for (int i = 0; i < 6; i++)
			{
				m_device.texture_data(m_prefilteredMap, mip, TextureType::TEXTURECUBE_POSITIVE_X + i, data);
				std::string fileName = "prefilter/prefiltered_map_mip" + std::to_string(mip) + "_" + std::to_string(i) + ".hdr";
				stbi_write_hdr(fileName.c_str(), width, height, 3, data);
			}

			delete[]data;
		}
	}

	void saveIrradianceMap()
	{
		int width, height = 0;
		m_device.texture_extents(m_irradianceMap, 0, width, height);

		float* data = new float[width * height * 3];

		for (int i = 0; i < 6; i++)
		{
			m_device.texture_data(m_irradianceMap, 0, TextureType::TEXTURECUBE_POSITIVE_X + i, data);
			std::string fileName = "prefilter/irradiance_map_" + std::to_string(i) + ".hdr";
			stbi_write_hdr(fileName.c_str(), width, height, 3, data);
		}

		delete[]data;
	}

    bool init() override
    {
		//m_Mat = dw::Material::Load("material/mat_rusted_iron.json", &m_device);
		//m_Mesh = dw::Mesh::Load("mesh/shaderBall.tsm", &m_device, m_Mat);
		
        m_camera = new Camera(45.0f,
                            0.1f,
                            1000.0f,
                            ((float)m_width)/((float)m_height),
                            glm::vec3(0.0f, 0.0f, 10.0f),
                            glm::vec3(0.0f, 0.0f, -1.0f));
        
        if(!createGeometry())
            return false;
    
        if(!compileShaders())
            return false;

		createSphereTransforms();
        
        BufferCreateDesc per_frame_ubo_desc;
		DW_ZERO_MEMORY(per_frame_ubo_desc);
        per_frame_ubo_desc.data = nullptr;
        per_frame_ubo_desc.data_type = DataType::FLOAT;
        per_frame_ubo_desc.size = sizeof(PerFrameUniforms);
        per_frame_ubo_desc.usage_type = BufferUsageType::DYNAMIC;
        
		m_uboAlign = m_device.uniform_buffer_alignment();

        BufferCreateDesc per_entity_ubo_desc;
		DW_ZERO_MEMORY(per_entity_ubo_desc);
        per_entity_ubo_desc.data = nullptr;
        per_entity_ubo_desc.data_type = DataType::FLOAT;
        per_entity_ubo_desc.size = m_uboAlign * NUM_ROWS * NUM_COLUMNS;
        per_entity_ubo_desc.usage_type = BufferUsageType::DYNAMIC;
        
        BufferCreateDesc per_scene_ubo_desc;
		DW_ZERO_MEMORY(per_scene_ubo_desc);
        per_scene_ubo_desc.data = nullptr;
        per_scene_ubo_desc.data_type = DataType::FLOAT;
        per_scene_ubo_desc.size = sizeof(PerSceneUniforms);
        per_scene_ubo_desc.usage_type = BufferUsageType::DYNAMIC;

		BufferCreateDesc cubemapUboDesc;
		DW_ZERO_MEMORY(cubemapUboDesc);
		cubemapUboDesc.data = nullptr;
		cubemapUboDesc.data_type = DataType::FLOAT;
		cubemapUboDesc.size = m_uboAlign * 6;
		cubemapUboDesc.usage_type = BufferUsageType::DYNAMIC;
        
        m_per_frame_ubo = m_device.create_uniform_buffer(per_frame_ubo_desc);
        m_per_entity_ubo = m_device.create_uniform_buffer(per_entity_ubo_desc);
        m_per_scene_ubo = m_device.create_uniform_buffer(per_scene_ubo_desc);
		m_cubeMapUBO = m_device.create_uniform_buffer(cubemapUboDesc);
        
        if (!m_per_frame_ubo || !m_per_entity_ubo)
        {
            LOG_FATAL("Failed to create Uniform Buffers");
            return 1;
        }
        
        RasterizerStateCreateDesc rs_desc;
		DW_ZERO_MEMORY(rs_desc);
        rs_desc.cull_mode = CullMode::NONE;
        rs_desc.fill_mode = FillMode::SOLID;
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
        
		m_per_scene.pointLightCount = 4;

        m_per_scene.pointLights[0].position = glm::vec4(-10.0f, 20.0f, 10.0f, 1.0f);
        m_per_scene.pointLights[0].color = glm::vec4(300.0f);

		m_per_scene.pointLights[1].position = glm::vec4(10.0f, 20.0f, 10.0f, 1.0f);
		m_per_scene.pointLights[1].color = glm::vec4(300.0f);

		m_per_scene.pointLights[2].position = glm::vec4(-10.0f, -20.0f, 10.0f, 1.0f);
		m_per_scene.pointLights[2].color = glm::vec4(300.0f);

		m_per_scene.pointLights[3].position = glm::vec4(10.0f, -20.0f, 10.0f, 1.0f);
		m_per_scene.pointLights[3].color = glm::vec4(300.0f);
	
		m_cube_transform = glm::mat4(1.0f);
		m_cube_transform = glm::scale(m_cube_transform, m_cube_scale);
        
		if (!createFramebuffers())
			return false;
        
        SamplerStateCreateDesc ssDesc;
        DW_ZERO_MEMORY(ssDesc);
        
        ssDesc.max_anisotropy = 0;
        ssDesc.min_filter = TextureFilteringMode::LINEAR;
        ssDesc.mag_filter = TextureFilteringMode::LINEAR;
        ssDesc.wrap_mode_u = TextureWrapMode::CLAMP_TO_EDGE;
        ssDesc.wrap_mode_v = TextureWrapMode::CLAMP_TO_EDGE;
        ssDesc.wrap_mode_w = TextureWrapMode::CLAMP_TO_EDGE;
        
        m_sampler = m_device.create_sampler_state(ssDesc);

		ssDesc.min_filter = TextureFilteringMode::LINEAR_ALL;
		ssDesc.mag_filter = TextureFilteringMode::LINEAR_ALL;

		m_cubemapSampler = m_device.create_sampler_state(ssDesc);

		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		glm::mat4 captureViews[] =
		{
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};

		for (int i = 0; i < 6; i++)
		{
			m_cubemapUniforms[i].proj = captureProjection;
			m_cubemapUniforms[i].view = captureViews[i];
		}

		loadHDRImage();

		m_device.wait_for_idle();

		latlongToCubeMap();
        
		m_device.wait_for_idle();

		//saveEnvMap();

		renderIrradianceMap();

		m_device.wait_for_idle();

		//saveIrradianceMap();

		renderPrefilteredMap();

		m_device.wait_for_idle();

		//savePrefilterMap();

		preintegrateBRDF();

		m_device.wait_for_idle();

		//saveBRDF();

        return true;
    }

	void createSphereTransforms()
	{
		const float spacing = 2.5f;

		float halfX = (spacing * (NUM_COLUMNS - 1)) / 2.0f;
		float halfY = (spacing  * (NUM_ROWS - 1)) / 2.0f;

		glm::vec4 startPos = glm::vec4(-halfX, -halfY, 0.0f, 0.0f);

		for (uint32_t i = 0; i < NUM_ROWS; i++)
		{
			for (uint32_t j = 0; j < NUM_COLUMNS; j++)
			{
				float metalness = i / float(NUM_ROWS - 1);
				float roughness = j / float(NUM_COLUMNS - 1);

				m_sphereUniforms[(i * NUM_COLUMNS) + j].u_pos = startPos;
				m_sphereUniforms[(i * NUM_COLUMNS) + j].u_model_mat = glm::translate(glm::mat4(1.0f), glm::vec3(startPos.x, startPos.y, startPos.z));
				m_sphereUniforms[(i * NUM_COLUMNS) + j].u_metalRough.x = metalness;
				m_sphereUniforms[(i * NUM_COLUMNS) + j].u_metalRough.y = glm::clamp(roughness, 0.05f, 1.0f);
				startPos += glm::vec4(spacing, 0.0f, 0.0f, 0.0f);
			}

			startPos += glm::vec4(0.0f, spacing, 0.0f, 0.0f);
			startPos.x = -halfX;
		}
	}
    
    bool compileShaders()
    {
        std::string vs_str;
        Utility::ReadText("shader/pbr_vs.glsl", vs_str);
        
        std::string fs_str;
        Utility::ReadText("shader/pbr_fs.glsl", fs_str);
        
        m_vs = m_device.create_shader(vs_str.c_str(), ShaderType::VERTEX);
        m_fs = m_device.create_shader(fs_str.c_str(), ShaderType::FRAGMENT);
        
        if (!m_vs || !m_fs)
        {
            LOG_FATAL("Failed to create Shaders");
            return false;
        }
        
        Shader* shaders[] = { m_vs, m_fs };
        m_program = m_device.create_shader_program(shaders, 2);
        
        if (!m_program)
        {
            LOG_FATAL("Failed to create Shader Program");
            return false;
        }
        
        vs_str.clear();
        Utility::ReadText("shader/quad_vs.glsl", vs_str);
        
        fs_str.clear();
        Utility::ReadText("shader/quad_fs.glsl", fs_str);
        
        m_quadVs = m_device.create_shader(vs_str.c_str(), ShaderType::VERTEX);
        m_quadFs = m_device.create_shader(fs_str.c_str(), ShaderType::FRAGMENT);
        
        if (!m_quadVs || !m_quadFs)
        {
            LOG_FATAL("Failed to create Shaders");
            return false;
        }
        
        Shader* quadShaders[] = { m_quadVs, m_quadFs };
        m_quadProgram = m_device.create_shader_program(quadShaders, 2);
        
        if (!m_quadProgram)
        {
            LOG_FATAL("Failed to create Shader Program");
            return false;
        }

		vs_str.clear();
		Utility::ReadText("shader/latlong_vs.glsl", vs_str);

		fs_str.clear();
		Utility::ReadText("shader/latlong_fs.glsl", fs_str);

		m_latlongToCubeVs = m_device.create_shader(vs_str.c_str(), ShaderType::VERTEX);
		m_latlongToCubeFs = m_device.create_shader(fs_str.c_str(), ShaderType::FRAGMENT);

		if (!m_latlongToCubeVs || !m_latlongToCubeFs)
		{
			LOG_FATAL("Failed to create Shaders");
			return false;
		}

		Shader* latlongShaders[] = { m_latlongToCubeVs, m_latlongToCubeFs };
		m_latlongToCubeProgram = m_device.create_shader_program(latlongShaders, 2);

		if (!m_latlongToCubeProgram)
		{
			LOG_FATAL("Failed to create Shader Program");
			return false;
		}
        
		vs_str.clear();
		Utility::ReadText("shader/cubemap_vs.glsl", vs_str);

		fs_str.clear();
		Utility::ReadText("shader/cubemap_fs.glsl", fs_str);

		m_cubeMapVs = m_device.create_shader(vs_str.c_str(), ShaderType::VERTEX);
		m_cubeMapFs = m_device.create_shader(fs_str.c_str(), ShaderType::FRAGMENT);

		if (!m_cubeMapVs || !m_cubeMapFs)
		{
			LOG_FATAL("Failed to create Shaders");
			return false;
		}

		Shader* cubemapShaders[] = { m_cubeMapVs, m_cubeMapFs };
		m_cubeMapProgram = m_device.create_shader_program(cubemapShaders, 2);

		if (!m_cubeMapProgram)
		{
			LOG_FATAL("Failed to create Shader Program");
			return false;
		}

		fs_str.clear();
		Utility::ReadText("shader/irradiance_fs.glsl", fs_str);

		m_irradianceFs = m_device.create_shader(fs_str.c_str(), ShaderType::FRAGMENT);

		if (!m_irradianceFs)
		{
			LOG_FATAL("Failed to create Shaders");
			return false;
		}

		Shader* irradianceShaders[] = { m_latlongToCubeVs, m_irradianceFs };
		m_irradianceProgram = m_device.create_shader_program(irradianceShaders, 2);

		if (!m_irradianceProgram)
		{
			LOG_FATAL("Failed to create Shader Program");
			return false;
		}

		fs_str.clear();
		Utility::ReadText("shader/prefilter_fs.glsl", fs_str);

		m_prefilterFs = m_device.create_shader(fs_str.c_str(), ShaderType::FRAGMENT);

		if (!m_prefilterFs)
		{
			LOG_FATAL("Failed to create Shaders");
			return false;
		}

		Shader* prefilterShaders[] = { m_latlongToCubeVs, m_prefilterFs };
		m_prefilterProgram = m_device.create_shader_program(prefilterShaders, 2);

		if (!m_prefilterProgram)
		{
			LOG_FATAL("Failed to create Shader Program");
			return false;
		}

		fs_str.clear();
		Utility::ReadText("shader/precompute_fs.glsl", fs_str);

		m_brdfIntegrateFs = m_device.create_shader(fs_str.c_str(), ShaderType::FRAGMENT);

		if (!m_brdfIntegrateFs)
		{
			LOG_FATAL("Failed to create Shaders");
			return false;
		}

		Shader* brdfShaders[] = { m_quadVs, m_brdfIntegrateFs };
		m_brdfIntegrateProgram = m_device.create_shader_program(brdfShaders, 2);

		if (!m_brdfIntegrateProgram)
		{
			LOG_FATAL("Failed to create Shader Program");
			return false;
		}
        return true;
    }

	bool createSphere()
	{
		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uv;
		std::vector<glm::vec3> normals;
		std::vector<uint32_t> indices;

		const unsigned int X_SEGMENTS = 64;
		const unsigned int Y_SEGMENTS = 64;
		const float PI = 3.14159265359;
		for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
		{
			for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
			{
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI);
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

				positions.push_back(glm::vec3(xPos, yPos, zPos));
				uv.push_back(glm::vec2(xSegment, ySegment));
				normals.push_back(glm::vec3(xPos, yPos, zPos));
			}
		}

		bool oddRow = false;
		for (int y = 0; y < Y_SEGMENTS; ++y)
		{
			if (!oddRow) // even rows: y == 0, y == 2; and so on
			{
				for (int x = 0; x <= X_SEGMENTS; ++x)
				{
					indices.push_back(y       * (X_SEGMENTS + 1) + x);
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				}
			}
			else
			{
				for (int x = X_SEGMENTS; x >= 0; --x)
				{
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					indices.push_back(y       * (X_SEGMENTS + 1) + x);
				}
			}
			oddRow = !oddRow;
		}

		m_sphereIndexCount = indices.size();

		std::vector<float> data;
		for (int i = 0; i < positions.size(); ++i)
		{
			data.push_back(positions[i].x);
			data.push_back(positions[i].y);
			data.push_back(positions[i].z);
			if (uv.size() > 0)
			{
				data.push_back(uv[i].x);
				data.push_back(uv[i].y);
			}
			if (normals.size() > 0)
			{
				data.push_back(normals[i].x);
				data.push_back(normals[i].y);
				data.push_back(normals[i].z);
			}
		}

		BufferCreateDesc bc;
		InputLayoutCreateDesc ilcd;
		VertexArrayCreateDesc vcd;

		DW_ZERO_MEMORY(bc);
		bc.data = (float*)&data[0];
		bc.data_type = DataType::FLOAT;
		bc.size = sizeof(float) * data.size();
		bc.usage_type = BufferUsageType::STATIC;

		m_sphereVBO = m_device.create_vertex_buffer(bc);

		DW_ZERO_MEMORY(bc);
		bc.data = (float*)&indices[0];
		bc.data_type = DataType::UINT32;
		bc.size = sizeof(uint32_t)* indices.size();
		bc.usage_type = BufferUsageType::STATIC;

		m_sphereIBO = m_device.create_index_buffer(bc);

		InputElement elements[] =
		{
			{ 3, DataType::FLOAT, false, 0, "POSITION" },
			{ 2, DataType::FLOAT, false, sizeof(float) * 3, "TEXCOORD" },
	        { 3, DataType::FLOAT, false, sizeof(float) * 5, "NORMAL" }
		};

		DW_ZERO_MEMORY(ilcd);
		ilcd.elements = elements;
		ilcd.num_elements = 3;
		ilcd.vertex_size = sizeof(float) * 8;

		m_sphereIL = m_device.create_input_layout(ilcd);

		DW_ZERO_MEMORY(vcd);
		vcd.index_buffer = m_sphereIBO;
		vcd.vertex_buffer = m_sphereVBO;
		vcd.layout = m_sphereIL;

		m_sphereVAO= m_device.create_vertex_array(vcd);

		if (!m_sphereVBO || !m_sphereIBO || !m_sphereVAO)
		{
			LOG_FATAL("Failed to create Vertex Buffers/Arrays");
			return false;
		}

		return true;
	}

	bool createCube()
	{
		float cubeVertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
																  // front face
			 -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			 -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
																  // right face
		 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
		 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
		 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
		 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
		 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
		 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
															   // bottom face
		-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
		1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
		1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
		1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
		-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
		-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
		// top face
		-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
		1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
		1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
		1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
		-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
		-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};

		BufferCreateDesc bc;
		InputLayoutCreateDesc ilcd;
		VertexArrayCreateDesc vcd;

		DW_ZERO_MEMORY(bc);
		bc.data = (float*)&cubeVertices[0];
		bc.data_type = DataType::FLOAT;
		bc.size = sizeof(cubeVertices);
		bc.usage_type = BufferUsageType::STATIC;

		m_cubeVBO = m_device.create_vertex_buffer(bc);

		InputElement elements[] =
		{
			{ 3, DataType::FLOAT, false, 0, "POSITION" },
			{ 3, DataType::FLOAT, false, sizeof(float) * 3, "NORMAL" },
			{ 2, DataType::FLOAT, false, sizeof(float) * 6, "TEXCOORD" }
		};

		DW_ZERO_MEMORY(ilcd);
		ilcd.elements = elements;
		ilcd.num_elements = 3;
		ilcd.vertex_size = sizeof(float) * 8;

		m_cubeIL = m_device.create_input_layout(ilcd);

		DW_ZERO_MEMORY(vcd);
		vcd.index_buffer = nullptr;
		vcd.vertex_buffer = m_cubeVBO;
		vcd.layout = m_cubeIL;

		m_cubeVAO = m_device.create_vertex_array(vcd);

		if (!m_cubeVBO || !m_cubeVAO)
		{
			LOG_FATAL("Failed to create Vertex Buffers/Arrays");
			return false;
		}

		return true;
	}

	void loadHDRImage()
	{
		stbi_set_flip_vertically_on_load(true);
		int width, height, nrComponents;
		float *data = stbi_loadf("texture/newport_loft.hdr", &width, &height, &nrComponents, 0);

		Texture2DCreateDesc desc;
		DW_ZERO_MEMORY(desc);

		desc.width = width;
		desc.height = height;
		desc.format = TextureFormat::R16G16B16_FLOAT;
		desc.mipmap_levels = 10;
		desc.data = data;

		m_latLongMap = m_device.create_texture_2d(desc);

		stbi_image_free(data);
	}

	void renderToCubeMap(Framebuffer** fbos, uint32_t width, uint32_t height)
	{
		char* mem = (char*)m_device.map_buffer(m_cubeMapUBO, BufferMapType::WRITE);

		for (int i = 0; i < 6; i++)
		{
			size_t offset = m_uboAlign * i;
			memcpy(mem + offset, &m_cubemapUniforms[i], sizeof(CubeMapUniforms));
		}

		m_device.unmap_buffer(m_cubeMapUBO);

		for (int i = 0; i < 6; i++)
		{
			m_device.bind_framebuffer(fbos[i]);
			m_device.set_viewport(width, height, 0, 0);
			m_device.bind_uniform_buffer_range(m_cubeMapUBO, ShaderType::VERTEX, 0, m_uboAlign * i, sizeof(CubeMapUniforms));
			m_device.clear_framebuffer(ClearTarget::ALL, (float*)clear_color);
			renderCube();
		}
	}

	void renderCubeMap()
	{
		m_device.bind_shader_program(m_cubeMapProgram);
		m_device.bind_uniform_buffer(m_per_frame_ubo, ShaderType::VERTEX, 0);
		m_device.bind_sampler_state(m_cubemapSampler, ShaderType::FRAGMENT, 0);
		//m_device.bind_texture(m_envMap, ShaderType::FRAGMENT, 0);
		//m_device.bind_texture(m_irradianceMap, ShaderType::FRAGMENT, 0);
		m_device.bind_texture(m_prefilteredMap, ShaderType::FRAGMENT, 0);
		//m_device.bind_framebuffer(m_offscreenFBO);
		m_device.bind_framebuffer(nullptr);
		m_device.set_viewport(m_width, m_height, 0, 0);
		renderCube();
	}

	void renderIrradianceMap()
	{
		m_device.bind_rasterizer_state(m_rs);
		m_device.bind_depth_stencil_state(m_ds);
		m_device.bind_shader_program(m_irradianceProgram);
		m_device.bind_sampler_state(m_sampler, ShaderType::FRAGMENT, 0);
		m_device.bind_texture(m_envMap, ShaderType::FRAGMENT, 0);
		renderToCubeMap(&m_irradianceMapFBOs[0], CUBEMAP_WIDTH, CUBEMAP_HEIGHT);
	}

	void renderPrefilteredMap()
	{
		char* mem = (char*)m_device.map_buffer(m_per_entity_ubo, BufferMapType::WRITE);

		if (mem)
		{
			// Write out roughness values
			for (int i = 0; i < PREFILTER_MIPMAPS; i++)
			{
				float roughness = (float)i / (float)(PREFILTER_MIPMAPS - 1);
				m_prefilterUniforms[i].u_metalRough.y = roughness;

				size_t offset = m_uboAlign * i;
				memcpy(mem + offset, &m_prefilterUniforms[i], sizeof(PerEntityUniforms));
			}

			m_device.unmap_buffer(m_per_entity_ubo);
		}

		m_device.bind_rasterizer_state(m_rs);
		m_device.bind_depth_stencil_state(m_ds);
		m_device.bind_shader_program(m_prefilterProgram);
		m_device.bind_sampler_state(m_cubemapSampler, ShaderType::FRAGMENT, 0);
		m_device.bind_texture(m_envMap, ShaderType::FRAGMENT, 0);

		for (int i = 0; i < PREFILTER_MIPMAPS; i++)
		{
			unsigned int mipWidth = PREFILTER_WIDTH * std::pow(0.5, i);
			unsigned int mipHeight = PREFILTER_HEIGHT * std::pow(0.5, i);
			m_device.bind_uniform_buffer_range(m_per_entity_ubo, ShaderType::FRAGMENT, 1, m_uboAlign * i, sizeof(PerEntityUniforms));
			renderToCubeMap(&m_prefilteredMapFBOs[i][0], mipWidth, mipHeight);
		}		
	}

	void preintegrateBRDF()
	{
		m_device.bind_framebuffer(m_brdfLUTFBO);
		m_device.set_viewport(512, 512, 0, 0);

		float clear[] = { 0.0f, 1.0f, 0.0f, 1.0f };
		m_device.clear_framebuffer(ClearTarget::ALL, clear);

		m_device.bind_shader_program(m_brdfIntegrateProgram);

		m_device.set_primitive_type(PrimitiveType::TRIANGLE_STRIP);
		m_device.bind_vertex_array(m_quadVAO);
		m_device.draw(0, 4);
	}

	void latlongToCubeMap()
	{
		m_device.bind_rasterizer_state(m_rs);
		m_device.bind_depth_stencil_state(m_ds);
		m_device.bind_shader_program(m_latlongToCubeProgram);
		m_device.bind_sampler_state(m_sampler, ShaderType::FRAGMENT, 0);
		m_device.bind_texture(m_latLongMap, ShaderType::FRAGMENT, 0);
		renderToCubeMap(&m_envMapFBOs[0], CUBEMAP_WIDTH, CUBEMAP_HEIGHT);

		m_device.generate_mipmaps(m_envMap);
	}

	bool createFramebuffers()
	{
		Texture2DCreateDesc rtDesc;
		DW_ZERO_MEMORY(rtDesc);
		rtDesc.format = TextureFormat::R8G8B8A8_UNORM;
		rtDesc.height = m_height;
		rtDesc.width = m_width;
		rtDesc.mipmap_levels = 10;

		m_textureRT = m_device.create_texture_2d(rtDesc);

		if (!m_textureRT)
		{
			std::cout << "Failed to create render target" << std::endl;
			return false;
		}

		rtDesc.format = TextureFormat::D32_FLOAT_S8_UINT;

		m_textureDS = m_device.create_texture_2d(rtDesc);

		if (!m_textureDS)
		{
			std::cout << "Failed to create depth target" << std::endl;
			return false;
		}

		rtDesc.format = TextureFormat::R32G32_FLOAT;
		rtDesc.height = 512;
		rtDesc.width = 512;

		m_brdfLUT = m_device.create_texture_2d(rtDesc);

		if (!m_brdfLUT)
		{
			std::cout << "Failed to create BRDF LUT" << std::endl;
			return false;
		}

		FramebufferCreateDesc fbDesc;
		DW_ZERO_MEMORY(fbDesc);
		fbDesc.renderTargetCount = 1;

		fbDesc.depthStencilTarget.texture = m_textureDS;
		fbDesc.depthStencilTarget.arraySlice = 0;
		fbDesc.depthStencilTarget.mipSlice = 0;

		fbDesc.renderTargets[0].texture = m_textureRT;
		fbDesc.renderTargets[0].arraySlice = 0;
		fbDesc.renderTargets[0].mipSlice = 0;

		m_offscreenFBO = m_device.create_framebuffer(fbDesc);

		if (!m_offscreenFBO)
		{
			std::cout << "Failed to create Framebuffer" << std::endl;
			return false;
		}

		fbDesc.renderTargets[0].texture = m_brdfLUT;
		fbDesc.renderTargets[0].arraySlice = 0;
		fbDesc.renderTargets[0].mipSlice = 0;

		m_brdfLUTFBO = m_device.create_framebuffer(fbDesc);

		if (!m_brdfLUTFBO)
		{
			std::cout << "Failed to create Framebuffer" << std::endl;
			return false;
		}

		TextureCubeCreateDesc desc;
		DW_ZERO_MEMORY(desc);

		desc.width = CUBEMAP_WIDTH;
		desc.height = CUBEMAP_HEIGHT;
		desc.mipmapLevels = PREFILTER_MIPMAPS;
		desc.format = TextureFormat::R32G32B32_FLOAT;

		m_envMap = m_device.create_texture_cube(desc);
		m_irradianceMap = m_device.create_texture_cube(desc);

		// Prefilter Map
		desc.width = PREFILTER_WIDTH;
		desc.height = PREFILTER_HEIGHT;
		desc.mipmapLevels = PREFILTER_MIPMAPS;

		m_prefilteredMap = m_device.create_texture_cube(desc);

		DW_ZERO_MEMORY(fbDesc);
		fbDesc.renderTargetCount = 1;

		fbDesc.renderTargets[0].texture = m_envMap;		
		fbDesc.renderTargets[0].mipSlice = 0;

		for (int i = 0; i < 6; i++)
		{
			fbDesc.renderTargets[0].arraySlice = TextureType::TEXTURECUBE + (i + 1);
			m_envMapFBOs[i] = m_device.create_framebuffer(fbDesc);
		}

		fbDesc.renderTargets[0].texture = m_irradianceMap;

		for (int i = 0; i < 6; i++)
		{
			fbDesc.renderTargets[0].arraySlice = TextureType::TEXTURECUBE + (i + 1);
			m_irradianceMapFBOs[i] = m_device.create_framebuffer(fbDesc);
		}

		// Prefilter Map
		fbDesc.renderTargets[0].texture = m_prefilteredMap;

		for (int i = 0; i < PREFILTER_MIPMAPS; i++)
		{
			fbDesc.renderTargets[0].mipSlice = i;

			for (int j = 0; j < 6; j++)
			{
				fbDesc.renderTargets[0].arraySlice = TextureType::TEXTURECUBE + (j + 1);
				m_prefilteredMapFBOs[i][j] = m_device.create_framebuffer(fbDesc);
			}
		}

		return true;
	}
    
    bool createGeometry()
    {
		// Create Sphere

		if (!createSphere())
			return false;

		// Create Cube

		if (!createCube())
			return false;

        // Create Quad

		BufferCreateDesc bc;
		InputLayoutCreateDesc ilcd;
		VertexArrayCreateDesc vcd;
        
        DW_ZERO_MEMORY(bc);
        bc.data = (float*)kQuadVertices;
        bc.data_type = DataType::FLOAT;
        bc.size = sizeof(kQuadVertices);
        bc.usage_type = BufferUsageType::STATIC;
        
        m_quadVBO = m_device.create_vertex_buffer(bc);
        
        InputElement quadElements[] =
        {
            { 3, DataType::FLOAT, false, 0, "POSITION" },
            { 2, DataType::FLOAT, false, sizeof(float) * 3, "TEXCOORD" }
            //        { 2, DataType::FLOAT, false, sizeof(float) * 3, "TEXCOORD" }
        };

        DW_ZERO_MEMORY(ilcd);
        ilcd.elements = quadElements;
        ilcd.num_elements = 2;
        ilcd.vertex_size = sizeof(float) * 5;
        
        m_quadLayout = m_device.create_input_layout(ilcd);
        
        DW_ZERO_MEMORY(vcd);
        vcd.index_buffer = nullptr; //ibo;
        vcd.vertex_buffer = m_quadVBO;
        vcd.layout = m_quadLayout;
        
        m_quadVAO = m_device.create_vertex_array(vcd);
        
        if (!m_quadVBO || !m_quadVAO)
        {
            LOG_FATAL("Failed to create Vertex Buffers/Arrays");
            return false;
        }

        return true;
    }

	void updateUniforms()
	{
		updateCamera();

		for (uint32_t i = 0; i < 4; i++)
		{
			m_per_scene.pointLights[i].position += glm::vec4(sin(glfwGetTime() * 5.0) * 0.5, 0.0, 0.0, 0.0);
		}

		//m_device.bind_framebuffer(m_offscreenFBO);
		m_device.bind_framebuffer(nullptr);
		m_device.set_viewport(m_width, m_height, 0, 0);
		m_device.clear_framebuffer(ClearTarget::ALL, (float*)clear_color);

		m_per_frame.u_proj_mat = m_camera->m_projection;
		m_per_frame.u_view_mat = m_camera->m_view;
		m_per_frame.u_view_pos = VEC3_TO_VEC4(m_camera->m_position);

		//ImGuizmo::BeginFrame();
		//ImGui::Begin("Matrix Inspector");
		//
		//EditTransform(glm::value_ptr(m_camera->m_view),
		//              glm::value_ptr(m_camera->m_projection),
		//              glm::value_ptr(m_cube_transform));
		//ImGui::End();

		m_per_entity.u_model_mat = m_cube_transform;
		m_per_entity.u_pos = VEC3_TO_VEC4(m_cube_position);



		void* mem = m_device.map_buffer(m_per_frame_ubo, BufferMapType::WRITE);

		if (mem)
		{
			memcpy(mem, &m_per_frame, sizeof(PerFrameUniforms));
			m_device.unmap_buffer(m_per_frame_ubo);
		}

		mem = m_device.map_buffer(m_per_scene_ubo, BufferMapType::WRITE);

		if (mem)
		{
			memcpy(mem, &m_per_scene, sizeof(PerSceneUniforms));
			m_device.unmap_buffer(m_per_scene_ubo);
		}
	}
    
    void update(double delta) override
    {
		//renderPrefilteredMap();

		updateUniforms();
         
        m_device.bind_rasterizer_state(m_rs);
        m_device.bind_depth_stencil_state(m_ds);
        
		renderSpheres();

		renderCubeMap();
		//dw::SubMesh* submeshes = m_Mesh->SubMeshes();
		//dw::Material* mat = m_Mesh->OverrideMaterial();

		//if (mat)
		//{
		//	Texture2D* albedo = mat->TextureAlbedo();

		//	if (albedo)
		//	{			
		//		m_device.bind_sampler_state(m_sampler, ShaderType::FRAGMENT, 0);
		//		m_device.bind_texture(albedo, ShaderType::FRAGMENT, 0);
		//	}

		//	Texture2D* normal = mat->TextureNormal();

		//	if (normal)
		//	{
		//		m_device.bind_sampler_state(m_sampler, ShaderType::FRAGMENT, 1);
		//		m_device.bind_texture(normal, ShaderType::FRAGMENT, 1);
		//	}
		//		
		//	Texture2D* metalness = mat->TextureMetalness();

		//	if (metalness)
		//	{	
		//		m_device.bind_sampler_state(m_sampler, ShaderType::FRAGMENT, 2);
		//		m_device.bind_texture(metalness, ShaderType::FRAGMENT, 2);
		//	}
		//		
		//	Texture2D* roughness = mat->TextureRoughness();

		//	if (roughness)
		//	{
		//		m_device.bind_sampler_state(m_sampler, ShaderType::FRAGMENT, 3);
		//		m_device.bind_texture(roughness, ShaderType::FRAGMENT, 3);
		//	}
		//}

		/*for (uint32_t i = 0; i < m_Mesh->SubMeshCount(); i++)
		{
			m_device.draw_indexed_base_vertex(submeshes[i].indexCount, submeshes[i].baseIndex, submeshes[i].baseVertex);
		}*/
        
  //      m_device.bind_framebuffer(nullptr);
  //      m_device.set_viewport(m_width, m_height, 0, 0);

  //      float clear[] = { 0.0f, 1.0f, 0.0f, 1.0f };
  //      m_device.clear_framebuffer(ClearTarget::ALL, clear);

  //      m_device.bind_shader_program(m_quadProgram);
	
  //      m_device.bind_sampler_state(m_cubemapSampler, ShaderType::FRAGMENT, 0);
		//m_device.bind_texture(m_textureRT, ShaderType::FRAGMENT, 0);
		////m_device.bind_texture(m_brdfLUT, ShaderType::FRAGMENT, 0);

  //      m_device.set_primitive_type(PrimitiveType::TRIANGLE_STRIP);
  //      m_device.bind_vertex_array(m_quadVAO);
  //      m_device.draw(0, 4);
//        
        //ImGui::ShowTestWindow();
    }

	void renderCube()
	{
		m_device.bind_vertex_array(m_cubeVAO);
		m_device.set_primitive_type(PrimitiveType::TRIANGLES);
		m_device.draw(0, 36);
	}

	void renderSpheres()
	{
		// Update Uniforms

		char* mem = (char*)m_device.map_buffer(m_per_entity_ubo, BufferMapType::WRITE);

		if (mem)
		{
			for (uint32_t i = 0; i < NUM_ROWS; i++)
			{
				for (uint32_t j = 0; j < NUM_COLUMNS; j++)
				{
					uint32_t index = (i * NUM_COLUMNS) + j;
					size_t offset = m_uboAlign * index;
					memcpy(mem + offset, &m_sphereUniforms[index], sizeof(PerEntityUniforms));
				}
			}

			m_device.unmap_buffer(m_per_entity_ubo);
		}

		m_device.bind_vertex_array(m_sphereVAO);
		m_device.bind_shader_program(m_program);
		m_device.bind_uniform_buffer(m_per_frame_ubo, ShaderType::VERTEX, 0);
		m_device.bind_uniform_buffer(m_per_scene_ubo, ShaderType::FRAGMENT, 2);

		m_device.bind_sampler_state(m_sampler, ShaderType::FRAGMENT, 0);
		m_device.bind_texture(m_irradianceMap, ShaderType::FRAGMENT, 0);

		m_device.bind_sampler_state(m_cubemapSampler, ShaderType::FRAGMENT, 1);
		m_device.bind_texture(m_prefilteredMap, ShaderType::FRAGMENT, 1);

		m_device.bind_sampler_state(m_sampler, ShaderType::FRAGMENT, 2);
		m_device.bind_texture(m_brdfLUT, ShaderType::FRAGMENT, 2);

		m_device.set_primitive_type(PrimitiveType::TRIANGLE_STRIP);
		
		for (uint32_t i = 0; i < NUM_ROWS; i++)
		{
			for (uint32_t j = 0; j < NUM_COLUMNS; j++)
			{
				m_device.bind_uniform_buffer_range(m_per_entity_ubo, ShaderType::VERTEX, 1, m_uboAlign * ((i * NUM_COLUMNS) + j), sizeof(PerEntityUniforms));
				m_device.draw_indexed(m_sphereIndexCount);
			}
		}
	}
    
    void shutdown() override
    {
		//dw::Material::Unload(m_Mat);
		//dw::Mesh::Unload(m_Mesh);

		for (int i = 0; i < 6; i++)
		{
			m_device.destroy(m_envMapFBOs[i]);
			m_device.destroy(m_irradianceMapFBOs[i]);

			for (int j = 0; j < PREFILTER_MIPMAPS; j++)
			{
				m_device.destroy(m_prefilteredMapFBOs[j][i]);
			}
		}

		m_device.destroy(m_brdfLUTFBO);
		m_device.destroy(m_prefilteredMap);
		m_device.destroy(m_latLongMap);
		m_device.destroy(m_envMap);
		m_device.destroy(m_irradianceMap);
		m_device.destroy(m_brdfLUT);
        m_device.destroy(m_sampler);
		m_device.destroy(m_cubemapSampler);
        m_device.destroy(m_ds);
        m_device.destroy(m_rs);
        m_device.destroy(m_per_scene_ubo);
        m_device.destroy(m_per_entity_ubo);
        m_device.destroy(m_per_frame_ubo);
		m_device.destroy(m_cubeMapUBO);
		m_device.destroy(m_prefilterProgram);
		m_device.destroy(m_prefilterFs);
		m_device.destroy(m_brdfIntegrateProgram);
		m_device.destroy(m_brdfIntegrateFs);
		m_device.destroy(m_irradianceProgram);
		m_device.destroy(m_irradianceFs);
		m_device.destroy(m_cubeMapProgram);
		m_device.destroy(m_cubeMapFs);
		m_device.destroy(m_cubeMapVs);
		m_device.destroy(m_latlongToCubeProgram);
		m_device.destroy(m_latlongToCubeFs);
		m_device.destroy(m_latlongToCubeVs);
        m_device.destroy(m_quadProgram);
        m_device.destroy(m_quadFs);
        m_device.destroy(m_quadVs);
        m_device.destroy(m_program);
        m_device.destroy(m_fs);
        m_device.destroy(m_vs);
        delete m_quadLayout;
		delete m_sphereIL;
		delete m_cubeIL;
        m_device.destroy(m_quadVAO);
        m_device.destroy(m_quadVBO);
		m_device.destroy(m_sphereVAO);
		m_device.destroy(m_sphereIBO);
		m_device.destroy(m_sphereVBO);
		m_device.destroy(m_cubeVAO);
		m_device.destroy(m_cubeVBO);
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
    
    //void EditTransform(const float *cameraView, float *cameraProjection, float* matrix)
    //{
    //    static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::ROTATE);
    //    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
    //    static bool useSnap = false;
    //    static float snap[3] = { 1.f, 1.f, 1.f };
    //    
    //    if (ImGui::IsKeyPressed(90))
    //        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    //    if (ImGui::IsKeyPressed(69))
    //        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    //    if (ImGui::IsKeyPressed(82)) // r Key
    //        mCurrentGizmoOperation = ImGuizmo::SCALE;
    //    if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
    //        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    //    ImGui::SameLine();
    //    if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
    //        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    //    ImGui::SameLine();
    //    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
    //        mCurrentGizmoOperation = ImGuizmo::SCALE;
    //    
    //    ImGui::InputFloat3("Tr", &m_cube_position.x, 3);
    //    ImGui::InputFloat3("Rt", &m_cube_rotation.x, 3);
    //    ImGui::InputFloat3("Sc", &m_cube_scale.x, 3);
    //    ImGuizmo::RecomposeMatrixFromComponents(&m_cube_position.x, &m_cube_rotation.x, &m_cube_scale.x, matrix);
    //    
    //    if (mCurrentGizmoOperation != ImGuizmo::SCALE)
    //    {
    //        if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
    //            mCurrentGizmoMode = ImGuizmo::LOCAL;
    //        ImGui::SameLine();
    //        if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
    //            mCurrentGizmoMode = ImGuizmo::WORLD;
    //    }
    //    if (ImGui::IsKeyPressed(83))
    //        useSnap = !useSnap;
    //    ImGui::Checkbox("", &useSnap);
    //    ImGui::SameLine();
    //    
    //    switch (mCurrentGizmoOperation)
    //    {
    //        case ImGuizmo::TRANSLATE:
    //            ImGui::InputFloat3("Snap", &snap[0]);
    //            break;
    //        case ImGuizmo::ROTATE:
    //            ImGui::InputFloat("Angle Snap", &snap[0]);
    //            break;
    //        case ImGuizmo::SCALE:
    //            ImGui::InputFloat("Scale Snap", &snap[0]);
    //            break;
    //    }
    //    ImGuiIO& io = ImGui::GetIO();
    //    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    //    ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, useSnap ? &snap[0] : NULL);
    //    
    //    ImGuizmo::DecomposeMatrixToComponents(matrix, &m_cube_position.x, &m_cube_rotation.x, &m_cube_scale.x);
    //}
};

DW_DECLARE_MAIN(Demo)
