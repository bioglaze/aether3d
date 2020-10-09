#include "PointLightComponent.hpp"
#include <locale>
#include <vector>
#include <string>
#include <sstream>

extern bool someLightCastsShadow;
std::vector< ae3d::PointLightComponent > pointLightComponents;
unsigned nextFreePointLightComponent = 0;

unsigned ae3d::PointLightComponent::New()
{
    if (nextFreePointLightComponent == pointLightComponents.size())
    {
        pointLightComponents.resize( pointLightComponents.size() + 10 );
    }
    
    return nextFreePointLightComponent++;
}

ae3d::PointLightComponent* ae3d::PointLightComponent::Get( unsigned index )
{
    return &pointLightComponents[ index ];
}

void ae3d::PointLightComponent::SetCastShadow( bool enable, int shadowMapSize )
{
    castsShadow = enable;
    const int mapSize = (shadowMapSize > 0 && shadowMapSize < 16385) ? shadowMapSize : 512;
    
    if (castsShadow && cachedShadowMapSize != mapSize)
    {
        someLightCastsShadow = true;
        cachedShadowMapSize = mapSize;
        shadowMap.CreateCube( mapSize, DataType::R32G32, TextureWrap::Clamp, TextureFilter::Linear, "pointlight shadow" );
    }
}

std::string GetSerialized( ae3d::PointLightComponent* component )
{
    std::stringstream outStream;
    std::locale c_locale( "C" );
    outStream.imbue( c_locale );

    auto color = component->GetColor();
    
    outStream << "pointlight\n";
    outStream << "shadow " << (component->CastsShadow() ? 1 : 0) << "\n";
    outStream << "radius " << component->GetRadius() << "\n";
    outStream << "pointlight_enabled " << component->IsEnabled() << "\n";
    outStream << "color " << color.x << " " << color.y << " " << color.z << "\n\n";

    return outStream.str();
}
