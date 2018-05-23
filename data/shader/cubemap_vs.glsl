layout (location = 0) in vec3 VS_IN_Position;
layout (location = 1) in vec3 VS_IN_Normal;
layout (location = 2) in vec2 VS_IN_TexCoord;

layout (std140) uniform u_PerFrame //#binding 0
{ 
	mat4 lastViewProj;
	mat4 viewProj;
	mat4 invViewProj;
	mat4 projMat;
	mat4 viewMat;
	vec4 viewPos;
	vec4 viewDir;
};

out vec3 PS_IN_Position;

void main()
{
    PS_IN_Position = VS_IN_Position;

    mat4 rotView = mat4(mat3(viewMat)); // remove translation from the view matrix
    vec4 clipPos = projMat * rotView * vec4(VS_IN_Position, 1.0);

    gl_Position = clipPos.xyww;
}