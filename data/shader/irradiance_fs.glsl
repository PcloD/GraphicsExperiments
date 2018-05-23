// ------------------------------------------------------------------
// OUTPUT VARIABLES  ------------------------------------------------
// ------------------------------------------------------------------

out vec4 PS_OUT_Color;

// ------------------------------------------------------------------
// INPUT VARIABLES  -------------------------------------------------
// ------------------------------------------------------------------

in vec3 PS_IN_Position;

// ------------------------------------------------------------------
// CONSTANTS  -------------------------------------------------------
// ------------------------------------------------------------------

const float kPI 	   = 3.14159265359;

// ------------------------------------------------------------------
// SAMPLERS  --------------------------------------------------------
// ------------------------------------------------------------------

uniform samplerCube s_EnvironmentMap; //#slot 0

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec3 normal = normalize(PS_IN_Position);
  
    vec3 irradiance = vec3(0.0);

    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(up, normal);
    up = cross(normal, right);

    float sampleDelta = 0.025;
    float numSamples = 0.0;

    for(float phi = 0.0; phi < 2.0 * kPI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * kPI; theta += sampleDelta)
        {
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

            irradiance += texture(s_EnvironmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            numSamples++;
        }
    }

    irradiance = kPI * irradiance * (1.0 / float(numSamples));
    PS_OUT_Color = vec4(irradiance, 1.0f);
}