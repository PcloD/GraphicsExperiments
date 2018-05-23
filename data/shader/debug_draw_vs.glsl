layout (location = 0) in vec3 VS_IN_Position;
layout (location = 1) in vec2 VS_IN_TexCoord;
layout (location = 2) in vec3 VS_IN_Color;

layout (std140) uniform CameraUniforms //#binding 0
{ 
	mat4 viewProj;
};

out vec3 PS_IN_Color;

void main()
{
    PS_IN_Color = VS_IN_Color;
    gl_Position = viewProj * vec4(VS_IN_Position, 1.0);
}