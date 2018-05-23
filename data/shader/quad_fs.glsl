out vec4 FragColor;

in vec2 PS_IN_TexCoord;

uniform sampler2D s_Color; //#slot 0

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