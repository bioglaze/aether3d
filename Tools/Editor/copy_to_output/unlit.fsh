#version 330 core

uniform sampler2D textureMap;
uniform sampler2D _ShadowMap;
uniform float _ShadowMinAmbient;
uniform int _LightType; // 0: None, 1: Spot, 2: Dir, 3: Point
uniform vec3 _LightColor;
uniform vec3 _LightPosition;
uniform vec3 _LightDirection;
uniform float _LightConeAngleCos;
uniform vec4 tint;

in vec3 vPositionWorld;
in vec2 vTexCoord;
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

out vec4 fragColor;

vec4 GetSpotLightAttenuation()
{
    vec3 vecToLight = normalize( _LightPosition - vPositionWorld );
    float spotAngle = dot( _LightDirection, vecToLight );

    // Falloff
    float dist = distance( vPositionWorld, _LightPosition );
    float a = dist / 15.0f /*_LightRadius*/ + 1.0;
    float att = 1.0f / (a * a);
    vec3 color = _LightColor * att;

    // Edge smoothing begins.
    float dif = 1.0f - _LightConeAngleCos;
    float factor = clamp( (spotAngle - _LightConeAngleCos) / dif, 0.0f, 1.0f );
    float linearFalloff = 0.04f;
    vec3 edgeColor = _LightColor * factor / (dist * linearFalloff);
    color = min( edgeColor, color );
    // Edge smoothing ends.

    return spotAngle > _LightConeAngleCos ? vec4( color, 1.0 ) : vec4( 0.0, 0.0, 0.0, 1.0 );
}

void main()
{
    float shadow = 1.0;
    float depth = vProjCoord.z / vProjCoord.w;
    shadow = max( _ShadowMinAmbient, VSM( depth ) );

    vec4 attenuatedLightColor = vec4( 1.0f, 1.0f, 1.0f, 1.0f );

    if (_LightType == 1)
    {
        attenuatedLightColor = GetSpotLightAttenuation();
    }

    fragColor = tint * texture( textureMap, vTexCoord ) * attenuatedLightColor * shadow;
}

