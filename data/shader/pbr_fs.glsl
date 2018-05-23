// ------------------------------------------------------------------
// STRUCTURES -------------------------------------------------------
// ------------------------------------------------------------------

#define MAX_POINT_LIGHTS 32

struct PointLight
{
	vec4 position;
	vec4 color;
};

struct DirectionalLight
{
	vec4 direction;
	vec4 color;
};

// ------------------------------------------------------------------
// UNIFORM BUFFERS --------------------------------------------------
// ------------------------------------------------------------------

layout (std140) uniform u_PerEntity //#binding 1
{
	mat4 mvpMat;
	mat4 modalMat;	
	vec4 worldPos;
	vec4 metalRough;
};


layout (std140) uniform u_PerScene //#binding 2
{
	PointLight 		 pointLights[MAX_POINT_LIGHTS];
	DirectionalLight directionalLight;
	int				 pointLightCount;
};

// ------------------------------------------------------------------
// CONSTANTS  -------------------------------------------------------
// ------------------------------------------------------------------

const float kPI 	   = 3.14159265359;
const float kMaxLOD    = 6.0;

// ------------------------------------------------------------------
// SAMPLERS  --------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler2D s_Albedo; //#slot 0
uniform sampler2D s_Normal; //#slot 1
uniform sampler2D s_Metalness; //#slot 2
uniform sampler2D s_Roughness; //#slot 3
uniform samplerCube s_IrradianceMap; //#slot 4
uniform samplerCube s_PrefilteredMap; //#slot 5
uniform sampler2D s_BRDF; //#slot 6

// ------------------------------------------------------------------
// INPUT VARIABLES  -------------------------------------------------
// ------------------------------------------------------------------

in vec3 PS_IN_Position;
in vec3 PS_IN_CamPos;
in vec3 PS_IN_Normal;
in vec2 PS_IN_TexCoord;

// ------------------------------------------------------------------
// OUTPUT VARIABLES  ------------------------------------------------
// ------------------------------------------------------------------

layout (location = 0) out vec4 PS_OUT_Color;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(s_Normal, PS_IN_TexCoord).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(PS_IN_Position);
    vec3 Q2  = dFdy(PS_IN_Position);
    vec2 st1 = dFdx(PS_IN_TexCoord);
    vec2 st2 = dFdy(PS_IN_TexCoord);

    vec3 N   = normalize(PS_IN_Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

vec3 FresnelSchlickRoughness(float HdotV, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - HdotV, 5.0);
}

float DistributionTrowbridgeReitzGGX(float NdotH, float roughness)
{
	// a = Roughness
	float a = roughness * roughness;
	float a2 = a * a;

	float numerator = a2;
	float denominator = ((NdotH * NdotH) * (a2 - 1.0) + 1.0);
	denominator = kPI * denominator * denominator;

	return numerator / denominator;
}

float GeometrySchlickGGX(float costTheta, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;

	float numerator = costTheta;
	float denominator = costTheta * (1.0 - k) + k;
	return numerator / denominator;
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
	float G1 = GeometrySchlickGGX(NdotV, roughness);
	float G2 = GeometrySchlickGGX(NdotL, roughness);
	return G1 * G2;
}

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
	const float kAmbient   = 1.0;

	// float kMetalness = metalRough.x;
	// float kRoughness = metalRough.y;

	vec4 kAlbedoAlpha = texture(s_Albedo, PS_IN_TexCoord);
	float kMetalness = texture(s_Metalness, PS_IN_TexCoord).x; 
	float kRoughness = texture(s_Roughness, PS_IN_TexCoord).x; 

	if (kAlbedoAlpha.w < 0.1)
		discard;

	vec3 kAlbedo = kAlbedoAlpha.xyz;

	vec3 N = getNormalFromMap();
	vec3 V = normalize(PS_IN_CamPos - PS_IN_Position); // FragPos -> ViewPos vector
	vec3 R = reflect(-V, N); 
	vec3 Lo = vec3(0.0);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, kAlbedo, kMetalness);

	float NdotV = max(dot(N, V), 0.0);
	vec3  F = FresnelSchlickRoughness(NdotV, F0, kRoughness);

	// For each directional light...
	{
		vec3 L = normalize(-directionalLight.direction.xyz); // FragPos -> LightPos vector
		vec3 H = normalize(V + L);
		float HdotV = clamp(dot(H, V), 0.0, 1.0);
		float NdotH = max(dot(N, H), 0.0);
		float NdotL = max(dot(N, L), 0.0);

		// Radiance -----------------------------------------------------------------

		vec3 Li = directionalLight.color.xyz * 20;

		// --------------------------------------------------------------------------

		// Specular Term ------------------------------------------------------------
		float D = DistributionTrowbridgeReitzGGX(NdotH, kRoughness);
		float G = GeometrySmith(NdotV, NdotL, kRoughness);

		vec3 numerator = D * G * F;
		float denominator = 4.0 * NdotV * NdotL; 

		vec3 specular = numerator / max(denominator, 0.001);
		// --------------------------------------------------------------------------

		// Diffuse Term -------------------------------------------------------------
		vec3 diffuse = kAlbedo / kPI;
		// --------------------------------------------------------------------------

		// Combination --------------------------------------------------------------
		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - kMetalness;

		Lo += (kD * kAlbedo / kPI + specular) * Li * NdotL;
		// --------------------------------------------------------------------------
	}

	// For each point light...
	for (int i = 0; i < pointLightCount; i++)
	{
		vec3 L = normalize(pointLights[i].position.xyz - PS_IN_Position); // FragPos -> LightPos vector
		vec3 H = normalize(V + L);
		float HdotV = clamp(dot(H, V), 0.0, 1.0);
		float NdotH = max(dot(N, H), 0.0);
		float NdotL = max(dot(N, L), 0.0);

		// Radiance -----------------------------------------------------------------

		float distance = length(pointLights[i].position.xyz - PS_IN_Position);
		float attenuation = 1.0 / (distance * distance);
		vec3 Li = pointLights[i].color.xyz * attenuation;

		// --------------------------------------------------------------------------

		// Specular Term ------------------------------------------------------------
		float D = DistributionTrowbridgeReitzGGX(NdotH, kRoughness);
		float G = GeometrySmith(NdotV, NdotL, kRoughness);

		vec3 numerator = D * G * F;
		float denominator = 4.0 * NdotV * NdotL; 

		vec3 specular = numerator / max(denominator, 0.001);
		// --------------------------------------------------------------------------

		// Diffuse Term -------------------------------------------------------------
		vec3 diffuse = kAlbedo / kPI;
		// --------------------------------------------------------------------------

		// Combination --------------------------------------------------------------
		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - kMetalness;

		Lo += (kD * kAlbedo / kPI + specular) * Li * NdotL;
		// --------------------------------------------------------------------------
	}

	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - kMetalness;

	vec3 irradiance = texture(s_IrradianceMap, N).rgb;
	vec3 diffuse = irradiance * kAlbedo;

	// Sample prefilter map and BRDF LUT
	vec3 prefilteredColor = textureLod(s_PrefilteredMap, R, kRoughness * kMaxLOD).rgb;
	vec2 brdf = texture(s_BRDF, vec2(max(NdotV, 0.0), kRoughness)).rg;
	vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular) * kAmbient;
	// vec3 ambient = vec3(0.03) * diffuse * kAmbient;

	vec3 color = Lo + ambient;

	// Gamma Correction
	color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  

    PS_OUT_Color = vec4(color, 1.0);
}