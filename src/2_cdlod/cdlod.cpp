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

#include <macros.h>

#define CAMERA_SPEED 0.05f
#define CAMERA_SENSITIVITY 0.02f
#define CAMERA_ROLL 0.0


struct Line
{
    glm::vec3 a;
    glm::vec3 b;
    
    Line(glm::vec3 _a, glm::vec3 _b) : a(_a), b(_b)
    {
        
    }
};

const Line kCube[] = {
    Line(glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f)),
    Line(glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(-1.0f, -1.0f, 1.0f)),
    Line(glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(-1.0f, -1.0f, 1.0f)),
    Line(glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(-1.0f, -1.0f, 1.0f))
};

class CDLOD : public dw::Application
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
        
        ImGuiIO& io = ImGui::GetIO();
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin("testing", nullptr, ImVec2(io.DisplaySize.x, io.DisplaySize.y), 0.0f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
        
        ImDrawList* list = ImGui::GetWindowDrawList();
        
        Line a = kCube[0];
        
        glm::vec4 a1 = m_camera->m_view_projection * glm::vec4(a.a.x, a.a.y, a.a.z, 1.0f);
        glm::vec4 a2 = m_camera->m_view_projection * glm::vec4(a.b.x, a.b.y, a.b.z, 1.0f);
        
        a1 /= a1.w;
        a2 /= a2.w;
        
        if ((a1.x <= 1.0f) && (a1.x >= -1.0f) && (a1.y <= 1.0f) && (a1.y >= -1.0f))
        {
            float x1 = (a1.x + 1.0f) / 2.0f;
            x1 *= m_width;
            float y1 = (a1.y + 1.0f) / 2.0f;
            y1 *= m_height;
            
            float x2 = (a2.x + 1.0f) / 2.0f;
            x2 *= m_width;
            float y2 = (a2.y + 1.0f) / 2.0f;
            y2 *= m_height;
            
            std::cout << x1 << ", " << y1 << std::endl;
            std::cout << x2 << ", " << y2 << std::endl;
            
            list->PathLineTo(ImVec2(x1, y1));
            list->PathLineTo(ImVec2(x2, y2));
            list->PathStroke(IM_COL32(0, 255, 0, 255), false);
        }
   
        ImGui::End();
        ImGui::PopStyleVar();
        
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

DW_DECLARE_MAIN(CDLOD)
