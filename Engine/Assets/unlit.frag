#version 450 core

layout (set = 0, binding = 1) uniform sampler2D textureMap;

layout (location = 0) in vec2 vTexCoord;
layout (location = 1) in vec4 vColor;
layout (location = 0) out vec4 fragColor;
        
void main()
{
    fragColor = texture( textureMap, vTexCoord );// * vColor;
}