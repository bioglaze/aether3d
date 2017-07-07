#version 450 core
    
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;

// Prevents generating code that needs ClipDistance which is not available.
out gl_PerVertex { vec4 gl_Position; };

layout (binding = 0) uniform UBO 
{
    uniform mat4 _ModelViewProjectionMatrix;
} ubo;

layout (location = 0) out vec2 vTexCoord;
layout (location = 1) out vec4 vColor;
    
void main()
{
    gl_Position = ubo._ModelViewProjectionMatrix * vec4( aPosition.xyz, 1.0 );

    vTexCoord = aTexCoord;
    vColor = aColor;
}
    