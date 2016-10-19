#version 330 core

uniform sampler2D textureMap;
uniform sampler2D _ShadowMap;
uniform sampler2D _ShadowMapCube;
uniform vec4 tint;

in vec2 vTexCoord;
#define SHADOW_MAP
#ifdef SHADOW_MAP
in vec4 vProjCoord;

float linstep(float low, float high, float v)
{
    return clamp( (v - low) / (high - low), 0.0, 1.0 );
}

float VSM( in float depth )
{
    vec2 moments = textureProj( _ShadowMap, vProjCoord ).rg;
    
    float variance = max( moments.y - moments.x * moments.x, -0.001 );
    
    float delta = depth - moments.x;
    float p = smoothstep( depth - 0.02, depth, moments.x );
    float pMax = linstep( 0.2, 1.0, variance / (variance + delta * delta) );
    
    return clamp( max( p, pMax ), 0.0, 1.0 );
}
#endif

out vec4 fragColor;

void main()
{
    float shadow = 1.0;
#ifdef SHADOW_MAP
    float depth = vProjCoord.z / vProjCoord.w;
    shadow = max( 0.2, VSM( depth ) );
#endif
    fragColor = tint * texture( textureMap, vTexCoord ) * shadow;
}

