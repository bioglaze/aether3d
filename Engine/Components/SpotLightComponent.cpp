#include "SpotLightComponent.hpp"
#include "System.hpp"
#include <string>

extern bool someLightCastsShadow;
static constexpr int MaxComponents = 100;
ae3d::SpotLightComponent spotLightComponents[ MaxComponents ];
unsigned nextFreeSpotlLightComponent = 0;

unsigned ae3d::SpotLightComponent::New()
{
    System::Assert( nextFreeSpotlLightComponent < MaxComponents, "Too many spot light components!" );
    
    if (nextFreeSpotlLightComponent == MaxComponents - 1)
    {
        return nextFreeSpotlLightComponent;
    }
    
    return nextFreeSpotlLightComponent++;
}

ae3d::SpotLightComponent* ae3d::SpotLightComponent::Get( unsigned index )
{
    return &spotLightComponents[ index ];
}

void ae3d::SpotLightComponent::SetCastShadow( bool enable, int shadowMapSize )
{
    castsShadow = enable;
    const int mapSize = (shadowMapSize > 0 && shadowMapSize < 16385) ? shadowMapSize : 512;
    
    if (castsShadow && cachedShadowMapSize != mapSize)
    {
        someLightCastsShadow = true;
        cachedShadowMapSize = mapSize;
        shadowMap.Create2D( mapSize, mapSize, RenderTexture::DataType::R32G32, TextureWrap::Clamp, TextureFilter::Linear, "spotlight shadow" );
    }
}

void ae3d::SpotLightComponent::SetConeAngle( float degrees )
{
    coneAngle = degrees;

    if (coneAngle < 0 || coneAngle > 180)
    {
        coneAngle = 35;
    }
}

std::string GetSerialized( ae3d::SpotLightComponent* component )
{
    std::string outStr = "spotlight\nshadow ";
    outStr += std::to_string( (component->CastsShadow() ? 1 : 0) );
    outStr += "\nconeAngle ";
    outStr += std::to_string( component->GetConeAngle() );
    outStr += "\nenabled ";
    outStr += std::to_string( (component->IsEnabled() ? 1 : 0) );
    outStr += "\nradius ";
    outStr += std::to_string( component->GetRadius() );
    outStr += "\ncolor ";
    auto color = component->GetColor();
    outStr += std::to_string( color.x );
    outStr += " ";
    outStr += std::to_string( color.y );
    outStr += " ";
    outStr += std::to_string( color.z );
    outStr += "\n\n";

    return outStr;
}
