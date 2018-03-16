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

#include <vec3.h>
#include <vector>

#include <Macros.h>

#define CAMERA_SPEED 0.05f
#define CAMERA_SENSITIVITY 0.02f
#define CAMERA_ROLL 0.0

namespace dd
{
    struct DW_ALIGNED(16) CameraUniforms
    {
        glm::mat4 view_proj;
    };
    
    struct VertexWorld
    {
        glm::vec3 position;
        glm::vec2 uv;
        glm::vec3 color;
    };
    
//    struct DrawCommand
//    {
//        int v0;
//        int v1;
//    };
    
#define MAX_VERTICES 100000
    
    class Renderer
    {
    private:
        CameraUniforms m_uniforms;
        VertexArray* m_line_vao;
        VertexBuffer* m_line_vbo;
        InputLayout* m_line_il;
        Shader* m_line_vs;
        Shader* m_line_fs;
        ShaderProgram* m_line_program;
        UniformBuffer* m_ubo;
        std::vector<VertexWorld> m_world_vertices;
//        std::vector<DrawCommand> m_draw_commands;
        RenderDevice* m_device;
        
    public:
        Renderer(RenderDevice* _device) : m_device(_device)
        {
            m_world_vertices.resize(MAX_VERTICES);
        }

        void line(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& c)
        {
            VertexWorld vw0, vw1;
            vw0.position = v0;
            vw0.color = c;
            
            vw1.position = v1;
            vw1.color = c;
            
            m_world_vertices.push_back(vw0);
            m_world_vertices.push_back(vw1);
        }
        
        void render(Framebuffer* fbo, int width, int height, const glm::mat4& view_proj)
        {
            void* ptr = m_device->map_buffer(m_line_vbo, BufferMapType::WRITE);
            memcpy(ptr, &m_world_vertices[0], sizeof(VertexWorld) * m_world_vertices.size());
            m_device->unmap_buffer(m_line_vbo);
            
            ptr = m_device->map_buffer(m_ubo, BufferMapType::WRITE);
            memcpy(ptr, &m_uniforms, sizeof(CameraUniforms));
            m_device->unmap_buffer(m_ubo);
            
            m_device->bind_framebuffer(fbo);
            m_device->set_viewport(width, height, 0, 0);
            m_device->bind_shader_program(m_line_program);
            m_device->bind_uniform_buffer(m_ubo, ShaderType::VERTEX, 0);
            m_device->bind_vertex_array(m_line_vao);
            m_device->set_primitive_type(PrimitiveType::LINES);
            
            for (int i = 0; i < m_world_vertices.size(); i += 2)
            {
                m_device->draw(i, 2);
            }
            
            m_world_vertices.clear();
        }
    };
}



class DebugDrawDemo : public dw::Application
{
private:
    Camera* m_camera;
    float m_heading_speed = 0.0f;
    float m_sideways_speed = 0.0f;
    bool m_mouse_look = false;

public:
    bool init() override
    {
        m_camera = new Camera(45.0f,
                              0.1f,
                              1000.0f,
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
    }
    
    void shutdown() override
    {
        
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

DW_DECLARE_MAIN(DebugDrawDemo)
