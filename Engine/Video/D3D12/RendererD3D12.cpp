#include "Renderer.hpp"
#include <string>
#include "System.hpp"

ae3d::Renderer renderer;

void ae3d::BuiltinShaders::Load()
{
    const std::string source(
        "struct VSOutput\
{\
    float4 pos : SV_POSITION;\
    float4 color : COLOR;\
    };\
    VSOutput VSMain( float3 pos : POSITION, float4 color : COLOR )\
    {\
        VSOutput vsOut;\
        vsOut.pos = float4( pos, 1.0 );\
        vsOut.color = color;\
        return vsOut;\
    }\
    float4 PSMain( VSOutput vsOut ) : SV_Target\
    {\
        return vsOut.color;\
    }"
        );

    spriteRendererShader.Load( source.c_str(), source.c_str() );

}
