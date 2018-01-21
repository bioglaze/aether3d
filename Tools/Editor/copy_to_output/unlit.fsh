#version 330 core

uniform sampler2D textureMap;
uniform sampler2D _ShadowMap;
uniform vec4 tint;

layout(std140) uniform PerObject
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
};

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

#if 0
vec4 GetSpotLightAttenuation()
{
    vec3 vecToLight = normalize( lightPosition - vPositionWorld );
    float spotAngle = dot( lightDirection, vecToLight );

    // Falloff
    float dist = distance( vPositionWorld, lightPosition );
    float a = dist / 15.0f /*_LightRadius*/ + 1.0;
    float att = 1.0f / (a * a);
    vec3 color = lightColor * att;

    // Edge smoothing begins.
    float dif = 1.0f - lightConeAngleCos;
    float factor = clamp( (spotAngle - lightConeAngleCos) / dif, 0.0f, 1.0f );
    float linearFalloff = 0.04f;
    vec3 edgeColor = lightColor * factor / (dist * linearFalloff);
    color = min( edgeColor, color );
    // Edge smoothing ends.

    return spotAngle > lightConeAngleCos ? vec4( color, 1.0 ) : vec4( 0.2, 0.2, 0.2, 1.0 );
}
#endif

void main()
{
    float shadow = 1.0;
    float depth = vProjCoord.z / vProjCoord.w;
    shadow = max( minAmbient, VSM( depth ) );

    /*vec4 attenuatedLightColor = vec4( 1.0f, 1.0f, 1.0f, 1.0f );

    if (_LightType == 1)
    {
        attenuatedLightColor = GetSpotLightAttenuation();
    }*/

    //fragColor = tint * texture( textureMap, vTexCoord ) * shadow;// * attenuatedLightColor;
    fragColor = texture( textureMap, vTexCoord ) * shadow;
}

