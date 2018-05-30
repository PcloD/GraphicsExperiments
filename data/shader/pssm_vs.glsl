// ------------------------------------------------------------------
// INPUT VARIABLES  -------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) in vec3  VS_IN_Position;
layout(location = 1) in vec2  VS_IN_TexCoord;
layout(location = 2) in vec3  VS_IN_Normal;
layout(location = 3) in vec3  VS_IN_Tangent;
layout(location = 4) in vec4  VS_IN_Bitangent;

// ------------------------------------------------------------------
// UNIFORM BUFFERS --------------------------------------------------
// ------------------------------------------------------------------

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

layout (std140) uniform u_PerEntity //#binding 1
{
	mat4 mvpMat;
	mat4 lastMvpMat;
	mat4 modelMat;	
	vec4 worldPos;
};

layout (std140) uniform u_PerFrustumSplit //#binding 2
{
	mat4 cropMat;
};

// ------------------------------------------------------------------
// MAIN  ------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
	vec4 pos = cropMat * modelMat * vec4(VS_IN_Position, 1.0f);
	gl_Position = pos;
}