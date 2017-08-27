#version 450 core

layout (location = 0) in vec3 vTexCoord;
layout (location = 0) out vec4 fragColor;

layout (set = 0, binding = 1) uniform samplerCube skyMap;

void main()
{
    fragColor = texture( skyMap, vTexCoord );
}
