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
    
    struct DrawCommand
    {
        int type;
        int vertices;
    };
    
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
        std::vector<DrawCommand> m_draw_commands;
        RenderDevice* m_device;
        RasterizerState* m_rs;
        DepthStencilState* m_ds;
        
    public:
        Renderer()
        {
            m_world_vertices.resize(MAX_VERTICES);
            m_world_vertices.clear();
            
            m_draw_commands.resize(MAX_VERTICES);
            m_draw_commands.clear();
        }
        
        bool init(RenderDevice* _device)
        {
            m_device = _device;
            
            std::string vs_str;
            Utility::ReadText("shader/debug_draw_vs.glsl", vs_str);
            
            std::string fs_str;
            Utility::ReadText("shader/debug_draw_fs.glsl", fs_str);
            
            m_line_vs = m_device->create_shader(vs_str.c_str(), ShaderType::VERTEX);
            m_line_fs = m_device->create_shader(fs_str.c_str(), ShaderType::FRAGMENT);
            
            if (!m_line_vs || !m_line_fs)
            {
                LOG_FATAL("Failed to create Shaders");
                return false;
            }
            
            Shader* shaders[] = { m_line_vs, m_line_fs };
            m_line_program = m_device->create_shader_program(shaders, 2);
            
            BufferCreateDesc bc;
            InputLayoutCreateDesc ilcd;
            VertexArrayCreateDesc vcd;
            
            DW_ZERO_MEMORY(bc);
            bc.data = nullptr;
            bc.data_type = DataType::FLOAT;
            bc.size = sizeof(VertexWorld) * MAX_VERTICES;
            bc.usage_type = BufferUsageType::DYNAMIC;
            
            m_line_vbo = m_device->create_vertex_buffer(bc);
            
            InputElement elements[] =
            {
                { 3, DataType::FLOAT, false, 0, "POSITION" },
                { 2, DataType::FLOAT, false, sizeof(float) * 3, "TEXCOORD" },
                { 3, DataType::FLOAT, false, sizeof(float) * 5, "COLOR" }
            };
            
            DW_ZERO_MEMORY(ilcd);
            ilcd.elements = elements;
            ilcd.num_elements = 3;
            ilcd.vertex_size = sizeof(float) * 8;
            
            m_line_il = m_device->create_input_layout(ilcd);
            
            DW_ZERO_MEMORY(vcd);
            vcd.index_buffer = nullptr;
            vcd.vertex_buffer = m_line_vbo;
            vcd.layout = m_line_il;
            
            m_line_vao = m_device->create_vertex_array(vcd);
            
            if (!m_line_vao || !m_line_vbo)
            {
                LOG_FATAL("Failed to create Vertex Buffers/Arrays");
                return false;
            }
            
            RasterizerStateCreateDesc rs_desc;
            DW_ZERO_MEMORY(rs_desc);
            rs_desc.cull_mode = CullMode::NONE;
            rs_desc.fill_mode = FillMode::SOLID;
            rs_desc.front_winding_ccw = true;
            rs_desc.multisample = false;
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
            uboDesc.size = sizeof(CameraUniforms);
            uboDesc.usage_type = BufferUsageType::DYNAMIC;
            
            m_ubo = m_device->create_uniform_buffer(uboDesc);
            
            return true;
        }
        
        void shutdown()
        {
            m_device->destroy(m_ubo);
            m_device->destroy(m_line_program);
            m_device->destroy(m_line_vs);
            m_device->destroy(m_line_fs);
            m_device->destroy(m_line_vbo);
            m_device->destroy(m_line_vao);
            m_device->destroy(m_ds);
            m_device->destroy(m_rs);
        }
        
        void capsule(const float& _height, const float& _radius, const glm::vec3& _pos, const glm::vec3& _c)
        {
            // Draw four lines
            line(glm::vec3(_pos.x, _pos.y + _radius, _pos.z-_radius), glm::vec3(_pos.x, _height - _radius, _pos.z-_radius), _c);
            line(glm::vec3(_pos.x, _pos.y + _radius, _pos.z+_radius), glm::vec3(_pos.x, _height - _radius, _pos.z+_radius), _c);
            line(glm::vec3(_pos.x-_radius, _pos.y + _radius, _pos.z), glm::vec3(_pos.x-_radius, _height - _radius, _pos.z), _c);
            line(glm::vec3(_pos.x+_radius, _pos.y + _radius, _pos.z), glm::vec3(_pos.x+_radius, _height - _radius, _pos.z), _c);
            
            float rad_45 = glm::radians(60.0f);
            float mid_y = _radius / 2.0f;//_radius * sin(rad_45);
            float mid_xz = _radius / 3.0f; // equal to mid_y because cos 45 = sin 45
            
            // Curve along z-axis
            line(glm::vec3(_pos.x, _height - _radius, _pos.z-_radius),                glm::vec3(_pos.x, _height - _radius + mid_y, _pos.z-_radius+mid_xz), _c);
            line(glm::vec3(_pos.x, _height - _radius + mid_y, _pos.z-_radius+mid_xz), glm::vec3(_pos.x, _height, _pos.z), _c);
            line(glm::vec3(_pos.x, _height - _radius, _pos.z+_radius),                glm::vec3(_pos.x, _height - _radius + mid_y, _pos.z+_radius-mid_xz), _c);
            line(glm::vec3(_pos.x, _height - _radius + mid_y, _pos.z+_radius-mid_xz), glm::vec3(_pos.x, _height, _pos.z), _c);
        }
        
        void aabb(const glm::vec3& _min, const glm::vec3& _max, const glm::vec3& _pos, const glm::vec3& _c)
        {
            glm::vec3 min = _pos + _min;
            glm::vec3 max = _pos + _max;
            
            line(min,                            glm::vec3(max.x, min.y, min.z), _c);
            line(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z), _c);
            line(glm::vec3(max.x, min.y, max.z), glm::vec3(min.x, min.y, max.z), _c);
            line(glm::vec3(min.x, min.y, max.z), min, _c);
            
            line(glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z), _c);
            line(glm::vec3(max.x, max.y, min.z), max, _c);
            line(max,                            glm::vec3(min.x, max.y, max.z), _c);
            line(glm::vec3(min.x, max.y, max.z), glm::vec3(min.x, max.y, min.z), _c);
            
            line(min,                            glm::vec3(min.x, max.y, min.z), _c);
            line(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z), _c);
            line(glm::vec3(max.x, min.y, max.z), max, _c);
            line(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z), _c);
        }
        
        void obb(const glm::vec3& min, const glm::vec3& max, const glm::mat4& model, const glm::vec3& c)
        {
            
        }
        
        void grid(const float& _x, const float& _z, const float& _y_level, const glm::vec3& _c)
        {
            int offset_x = floor(_x/2.0f);
            int offset_z = floor(_z/2.0f);
            
            for (int x = -offset_x; x < (offset_x + 1); x++)
            {
                line(glm::vec3(x, _y_level, -offset_z), glm::vec3(x, _y_level, offset_z), _c);
            }
            
            for (int z = -offset_z; z < (offset_z + 1); z++)
            {
                line(glm::vec3(-offset_x, _y_level, z), glm::vec3(offset_x, _y_level, z), _c);
            }
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
            
            DrawCommand cmd;
            cmd.type = PrimitiveType::LINES;
            cmd.vertices = 2;
            
            m_draw_commands.push_back(cmd);
        }
        
        void line_strip(glm::vec3* v, const int& count, const glm::vec3& c)
        {
            for (int i = 0; i < count; i++)
            {
                VertexWorld vert;
                vert.position = v[i];
                vert.color = c;
                
                m_world_vertices.push_back(vert);
            }
            
            DrawCommand cmd;
            cmd.type = PrimitiveType::LINE_STRIP;
            cmd.vertices = count;
            
            m_draw_commands.push_back(cmd);
        }
        
        void circle_xy(float radius, const glm::vec3& c)
        {
            glm::vec3 verts[19];
            
            int idx = 0;
            
            for (int i=0; i < 360; i+= 20)
            {
                float degInRad = glm::radians((float)i);
                verts[idx++] = glm::vec3(cos(degInRad)*radius,sin(degInRad)*radius, 0.0f);
            }
            
            verts[18] = verts[0];
            line_strip(&verts[0], 19, c);
        }
        
        void circle_xz(float radius, const glm::vec3& c)
        {
            glm::vec3 verts[19];
            
            int idx = 0;
            
            for (int i=0; i < 360; i+= 20)
            {
                float degInRad = glm::radians((float)i);
                verts[idx++] = glm::vec3(cos(degInRad)*radius, 0.0f, sin(degInRad)*radius);
            }
            
            verts[18] = verts[0];
            line_strip(&verts[0], 19, c);
        }
        
        void circle_yz(float radius, const glm::vec3& c)
        {
            glm::vec3 verts[19];
            
            int idx = 0;
            
            for (int i=0; i < 360; i+= 20)
            {
                float degInRad = glm::radians((float)i);
                verts[idx++] = glm::vec3(0.0f, cos(degInRad)*radius, sin(degInRad)*radius);
            }
            
            verts[18] = verts[0];
            line_strip(&verts[0], 19, c);
        }
        
        void render(Framebuffer* fbo, int width, int height, const glm::mat4& view_proj)
        {
            m_uniforms.view_proj = view_proj;
            
            void* ptr = m_device->map_buffer(m_line_vbo, BufferMapType::WRITE);
            
            if (m_world_vertices.size() > MAX_VERTICES)
                std::cout << "Vertices are above limit" << std::endl;
            else
                memcpy(ptr, &m_world_vertices[0], sizeof(VertexWorld) * m_world_vertices.size());
            
            m_device->unmap_buffer(m_line_vbo);
            
            ptr = m_device->map_buffer(m_ubo, BufferMapType::WRITE);
            memcpy(ptr, &m_uniforms, sizeof(CameraUniforms));
            m_device->unmap_buffer(m_ubo);
            
            m_device->bind_rasterizer_state(m_rs);
            m_device->bind_depth_stencil_state(m_ds);
            m_device->bind_framebuffer(fbo);
            m_device->set_viewport(width, height, 0, 0);
            m_device->bind_shader_program(m_line_program);
            m_device->bind_uniform_buffer(m_ubo, ShaderType::VERTEX, 0);
            m_device->bind_vertex_array(m_line_vao);
            
//            for (int i = 0; i < m_world_vertices.size(); i += 2)
//            {
//                m_device->draw(i, 2);
//            }
            
            int v = 0;
            
            for (int i = 0; i < m_draw_commands.size(); i++)
            {
                DrawCommand& cmd = m_draw_commands[i];
                m_device->set_primitive_type(cmd.type);
                m_device->draw(v, cmd.vertices);
                v += cmd.vertices;
            }
            
            m_draw_commands.clear();
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
    dd::Renderer m_debug_renderer;
    
    glm::vec3 m_min_extents;
    glm::vec3 m_max_extents;
    glm::vec3 m_pos;
    glm::vec3 m_color;

public:
    bool init() override
    {
        m_camera = new Camera(45.0f,
                              0.1f,
                              1000.0f,
                              ((float)m_width)/((float)m_height),
                              glm::vec3(0.0f, 0.0f, 10.0f),
                              glm::vec3(0.0f, 0.0f, -1.0f));
        
        m_min_extents = glm::vec3(-10.0f);
        m_max_extents = glm::vec3(10.0f);
        m_pos = glm::vec3(40.0f);
        m_color = glm::vec3(1.0f, 0.0f, 0.0f);
        
        return m_debug_renderer.init(&m_device);
    }
    
    void update(double delta) override
    {
        updateCamera();
        
        m_device.bind_framebuffer(nullptr);
        m_device.set_viewport(m_width, m_height, 0, 0);
        
        float clear[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        m_device.clear_framebuffer(ClearTarget::ALL, clear);
        
        ImGui::Begin("Debug Draw");
        
        ImGui::InputFloat3("Min Extents", &m_min_extents[0]);
        ImGui::InputFloat3("Max Extents", &m_max_extents[0]);
        ImGui::InputFloat3("Position", &m_pos[0]);
        ImGui::ColorEdit3("Color", &m_color[0]);
        
        ImGui::End();
        
        m_debug_renderer.circle_xy(20.0f, glm::vec3(1.0f, 0.0f, 0.0f));
        m_debug_renderer.circle_xz(20.0f, glm::vec3(1.0f, 0.0f, 0.0f));
        m_debug_renderer.circle_yz(20.0f, glm::vec3(1.0f, 0.0f, 0.0f));
        m_debug_renderer.capsule(20.0f, 2.0f, glm::vec3(-20.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0));
        //m_debug_renderer.grid(101.0f, 101.0f, 0.0f, glm::vec3(1.0f));
        m_debug_renderer.aabb(m_min_extents, m_max_extents, m_pos, m_color);
        
        m_debug_renderer.render(nullptr, m_width, m_height, m_camera->m_view_projection);
    }
    
    void shutdown() override
    {
        m_debug_renderer.shutdown();
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
