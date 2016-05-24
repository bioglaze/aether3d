#version 450

// Prevents generating code that needs ClipDistance which is not available.
out gl_PerVertex { vec4 gl_Position; };

layout (location = 0) in vec3 aPosition;

layout (binding = 0) uniform UBO 
{
    uniform mat4 _ModelViewProjectionMatrix;
} ubo;

layout (location = 0) out vec3 vTexCoord;

void main()
{
    gl_Position = ubo._ModelViewProjectionMatrix * vec4( aPosition.xyz, 1.0 );
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;

    vTexCoord = aPosition;
}
