layout (location = 0) in vec3 VS_IN_Position;
layout (location = 1) in vec3 VS_IN_Normal;
layout (location = 2) in vec2 VS_IN_TexCoord;

layout (std140) uniform u_CubeMapUniforms //#binding 0
{ 
	mat4 proj;
	mat4 view;
};

out vec3 PS_IN_Position;

void main()
{
    PS_IN_Position = VS_IN_Position;
    gl_Position = proj * view * vec4(VS_IN_Position, 1.0);
}