#include "Renderer.hpp"
#include <string>
#include "System.hpp"

ae3d::Renderer renderer;

void ae3d::BuiltinShaders::Load()
{
    const char* spriteSource =
        "struct VSOutput"
        "{"
        "    float4 pos : SV_POSITION;"
        "    float2 uv : TEXCOORD;"
        "    float4 color : COLOR;"
        "};"

        "cbuffer Scene"
        "{"
        "    float4x4 _ProjectionModelMatrix;"
        "};"

        "VSOutput VSMain( float3 pos : POSITION, float2 uv : TEXCOORD, float4 color : COLOR )"
        "{"
        "    VSOutput vsOut;"
        "    vsOut.pos = mul( _ProjectionModelMatrix, float4( pos, 1.0 ) );"
        "    vsOut.uv = uv;"
        "    vsOut.color = color;"
        "    return vsOut;"
        "}"

        "Texture2D<float4> tex : register(t0);"
        "SamplerState sLinear : register(s0);"

        "float4 PSMain( VSOutput vsOut ) : SV_Target"
        "{"
        "    return tex.SampleLevel( sLinear, vsOut.uv, 0 ) * vsOut.color;"
        "}";

    spriteRendererShader.Load( spriteSource, spriteSource );

    const std::string sdfSource(
        "struct VSOutput\
{\
    float4 pos : SV_POSITION;\
    float2 uv : TEXCOORD;\
    float4 color : COLOR;\
    };\
cbuffer Scene\
    {\
        float4x4 _ProjectionModelMatrix;\
    };\
    VSOutput VSMain( float3 pos : POSITION, float2 uv : TEXCOORD, float4 color : COLOR )\
    {\
        VSOutput vsOut;\
        vsOut.pos = mul( _ProjectionModelMatrix, float4( pos, 1.0 ) );\
        vsOut.uv = uv;\
        vsOut.color = color;\
        return vsOut;\
    }\
\
    Texture2D< float4 > tex : register(t0);\
    SamplerState sLinear : register(s0);\
\
    float4 PSMain( VSOutput vsOut ) : SV_Target\
    {\
        const float edgeDistance = 0.5;\
        float distance = tex.SampleLevel( sLinear, vsOut.uv, 0 );\
        float edgeWidth = 0.7 * length( float2( ddx( distance ), ddy( distance ) ) );\
        float opacity = smoothstep( edgeDistance - edgeWidth, edgeDistance + edgeWidth, distance );\
        return float4( vsOut.color.rgb, opacity );\
    }"
        );
    sdfShader.Load( sdfSource.c_str(), sdfSource.c_str() );

    // TODO: Implement
    const std::string skyboxSource(
        "struct VSOutput\
{\
    float4 pos : SV_POSITION;\
    float4 color : COLOR;\
    };\
cbuffer Scene\
    {\
        float4x4 _ModelViewProjectionMatrix;\
    };\
    VSOutput VSMain( float3 pos : POSITION, float2 uv : TEXCOORD, float4 color : COLOR )\
    {\
        VSOutput vsOut;\
        vsOut.pos = mul( _ModelViewProjectionMatrix, float4( pos, 1.0 ) );\
        vsOut.color = color;\
        return vsOut;\
    }\
    float4 PSMain( VSOutput vsOut ) : SV_Target\
    {\
        return float4(0.0, 1.0, 0.0, 1.0);\
    }"
        );

    skyboxShader.Load( skyboxSource.c_str(), skyboxSource.c_str() );
    
    const char* depthNormalsSource = R"(

        struct VSOutput
        {
            float4 pos : SV_POSITION;
            float3 mvPosition : POSITION;
            float3 normal : NORMAL;
        };

        cbuffer Scene
        {
            float4x4 _ModelViewProjectionMatrix;
            float4x4 _ModelViewMatrix;
        };

        VSOutput VSMain( float3 pos : POSITION, float3 normal : NORMAL )
        {
            VSOutput vsOut;
            vsOut.pos = mul( _ModelViewProjectionMatrix, float4( pos, 1.0 ) );
            vsOut.mvPosition = mul( _ModelViewMatrix, float4( pos, 1.0 ) ).xyz;
            vsOut.normal = mul( _ModelViewMatrix, float4( normal, 0.0 ) ).xyz;
            return vsOut;
        }

        float4 PSMain( VSOutput vsOut ) : SV_Target
        {
            float linearDepth = vsOut.mvPosition.z;
            return float4( linearDepth, vsOut.normal );
        }
    )";

    depthNormalsShader.Load( depthNormalsSource, depthNormalsSource );
}
