#version 450 core

layout (set = 0, binding = 1) uniform sampler2D textureMap;

layout (location = 0) in vec3 vPositionInView;
layout (location = 1) in vec3 vNormalInView;
layout (location = 0) out vec4 fragColor;
        
void main()
{
    float linearDepth = vPositionInView.z;
    fragColor = vec4( linearDepth, vNormalInView );
}