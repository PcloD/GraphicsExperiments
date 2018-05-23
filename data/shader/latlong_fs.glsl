out vec4 PS_OUT_Color;

in vec3 PS_IN_Position;

uniform sampler2D s_EnvironmentMap; //#slot 0

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(PS_IN_Position));
    vec3 color = texture2D(s_EnvironmentMap, uv).rgb;

    PS_OUT_Color = vec4(color, 1.0f);
}