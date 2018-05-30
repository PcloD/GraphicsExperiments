out vec4 FragColor;

in vec2 PS_IN_TexCoord;

uniform sampler2D s_Color; //#slot 0
uniform sampler2DArray s_ShadowMap; //#slot 1

#define MAX_SHADOW_FRUSTUM 8

struct ShadowFrustum
{
	mat4  shadowMatrix;
	float farPlane;
};

layout (std140) uniform u_PerFrame //#binding 0
{ 
	mat4 		  lastViewProj;
	mat4 		  viewProj;
	mat4 		  invViewProj;
	mat4 		  projMat;
	mat4 		  viewMat;
	vec4 		  viewPos;
	vec4 		  viewDir;
	int			  numCascades;
	ShadowFrustum shadowFrustums[MAX_SHADOW_FRUSTUM];
};

float GetLinearDepth()
{
    float f=1000.0;
    float n = 0.1;
    float z = (2 * n) / (f + n - texture( s_Color, PS_IN_TexCoord ).x * (f - n));
    return z;
}

void main()
{
    // float z = GetLinearDepth();
    // FragColor = vec4(z, z, z, 1.0f);
    FragColor = vec4(texture(s_Color, PS_IN_TexCoord).xyz, 1.0f);
}