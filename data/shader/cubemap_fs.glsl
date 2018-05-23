out vec4 PS_OUT_Color;

in vec3 PS_IN_Position;

uniform samplerCube s_Environment; //#slot 0

void main()
{
    vec3 envColor = texture(s_Environment, PS_IN_Position).rgb;
    //vec3 envColor = textureLod(s_Environment, PS_IN_Position, 4).rgb;
    
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2)); 
  
    PS_OUT_Color = vec4(envColor, 1.0);
}