#version 450 core

layout (location = 0) in vec3 aPosition;

layout (binding = 0) uniform UBO 
{
    uniform mat4 _ModelViewProjectionMatrix;
} ubo;

layout (location = 0) out vec3 vTexCoord;

void main()
{
    gl_Position = ubo._ModelViewProjectionMatrix * vec4( aPosition.xyz, 1.0 );
    vTexCoord = aPosition;
}
