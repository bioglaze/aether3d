#version 450 core
    
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;

// Prevents generating code that needs ClipDistance which is not available.
out gl_PerVertex { vec4 gl_Position; };

layout (set=0, binding = 0) uniform UBO 
{
    mat4 localToClip;
    mat4 localToView;
    mat4 localToWorld;
    mat4 localToShadowClip;
    mat4 clipToView;
    vec4 lightPosition;
    vec4 lightDirection;
    vec4 lightColor;
    float lightConeAngle;
    int lightType; // 0: None, 1: Spot, 2: Dir, 3: Point
    float minAmbient;
    uint maxNumLightsPerTile;
    uint windowWidth;
    uint windowHeight;
    uint numLights; // 16 bits for point light count, 16 for spot light count
    uint padding;
    mat4 boneMatrices[ 80 ];
} ubo;

layout (location = 0) out vec2 vTexCoord;
layout (location = 1) out vec4 vColor;
    
void main()
{
    gl_Position = ubo.localToClip * vec4( aPosition.xyz, 1.0 );

    vTexCoord = aTexCoord;
    vColor = aColor;
}
    