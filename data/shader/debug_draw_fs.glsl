out vec4 PS_OUT_Color;

in vec3 PS_IN_Color;

void main()
{
    PS_OUT_Color = vec4(PS_IN_Color, 1.0);
}