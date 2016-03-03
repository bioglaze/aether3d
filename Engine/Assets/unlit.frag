#version 450 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D textureMap;

layout (location = 0) in vec2 vTexCoord;
layout (location = 1) in vec4 vColor;
layout (location = 0) out vec4 fragColor;
        
void main()
{
    //fragColor = vec4( 1.0f, 0.0f, 0.f, 1.0f );//
    fragColor = texture( textureMap, vTexCoord );// * vColor;
}