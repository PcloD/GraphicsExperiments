// ------------------------------------------------------------------
// INPUT VARIABLES  -------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) in vec3  VS_IN_Position;
layout(location = 1) in vec3  VS_IN_Normal;

// ------------------------------------------------------------------
// UNIFORM BUFFERS --------------------------------------------------
// ------------------------------------------------------------------

layout (std140) uniform u_PerFrame //#binding 0
{ 
	mat4 u_last_vp_mat;
	mat4 u_vp_mat;
	mat4 u_inv_vp_mat;
	mat4 u_proj_mat;
	mat4 u_view_mat;
	vec4 u_view_pos;
	vec4 u_view_dir;
};

layout (std140) uniform u_PerEntity //#binding 1
{
	mat4 u_mvp_mat;
	mat4 u_model_mat;	
	vec4 u_pos;
};

// ------------------------------------------------------------------
// OUTPUT VARIABLES  ------------------------------------------------
// ------------------------------------------------------------------

out vec3 PS_IN_Position;
out vec3 PS_IN_Normal;

// ------------------------------------------------------------------
// MAIN  ------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
	vec4 pos = u_proj_mat * u_view_mat * u_model_mat * vec4(VS_IN_Position, 1.0f);

	PS_IN_Normal = VS_IN_Normal;
	PS_IN_Position = pos.xyz;
	
	gl_Position = pos;
}