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

#define CAMERA_SPEED 0.1f
#define CAMERA_SENSITIVITY 0.02f
#define CAMERA_ROLL 0.0

using namespace math;

const float kTRIANGLE_VERTS[] = { 
	-0.8f, -0.8f, 0.0f,
	0.0f, 0.8f, 0.0f,
	0.8f, -0.8f, 0.0f
};

class Tessellation : public dw::Application
{
private:
    Camera* m_camera;
    float m_heading_speed = 0.0f;
    float m_sideways_speed = 0.0f;
    bool m_mouse_look = false;

	VertexArray* m_vao;
	VertexBuffer* m_vbo;
	IndexBuffer* m_ibo;
	InputLayout* m_il;
	ShaderProgram* m_tess_program;
	Shader* m_vs;
	Shader* m_tcs;
	Shader* m_tes;
	Shader* m_fs;
	RasterizerState* m_rs;
	DepthStencilState* m_ds;

public:
    bool init() override
    {
        m_camera = new Camera(45.0f,
                              0.1f,
                              100000.0f,
                              ((float)m_width)/((float)m_height),
                              glm::vec3(0.0f, 0.0f, 10.0f),
                              glm::vec3(0.0f, 0.0f, -1.0f));
       
        return true;
    }
    
    void update(double delta) override
    {
        updateCamera();
        
        m_device.bind_framebuffer(nullptr);
        m_device.set_viewport(m_width, m_height, 0, 0);
        
        float clear[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        m_device.clear_framebuffer(ClearTarget::ALL, clear);
       
        ImGui::ShowDemoWindow();

		m_device.bind_shader_program(m_tess_program);
		m_device.bind_vertex_array(m_vao);
		m_device.bind_rasterizer_state(m_rs);
		m_device.bind_depth_stencil_state(m_ds);
		m_device.set_primitive_type(PrimitiveType::PATCHES);
		m_device.draw(0, 3);
    }
    
    void shutdown() override
    {
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

DW_DECLARE_MAIN(Tessellation)
