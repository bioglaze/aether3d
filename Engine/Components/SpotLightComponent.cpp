#include "SpotLightComponent.hpp"
#include <locale>
#include <string>
#include <vector>
#include <sstream>

std::vector< ae3d::SpotLightComponent > spotLightComponents;
unsigned nextFreeSpotlLightComponent = 0;

unsigned ae3d::SpotLightComponent::New()
{
    if (nextFreeSpotlLightComponent == spotLightComponents.size())
    {
        spotLightComponents.resize( spotLightComponents.size() + 10 );
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
    
    // TODO: create only if not already created with current size.
    if (castsShadow)
    {
        shadowMap.Create2D( mapSize, mapSize, RenderTexture::DataType::R32G32, TextureWrap::Clamp, TextureFilter::Linear, "spotlight shadow" );
    }
}

std::string GetSerialized( ae3d::SpotLightComponent* component )
{
    std::stringstream outStream;
    std::locale c_locale( "C" );
    outStream.imbue( c_locale );

    auto color = component->GetColor();
    
    outStream << "spotlight\n";
    outStream << "shadow " << (component->CastsShadow() ? 1 : 0) << "\n";
    outStream << "coneangle " << component->GetConeAngle() << "\n";
    outStream << "enabled" << component->IsEnabled() << "\n";
    outStream << "radius " << component->GetRadius() << "\n";
    outStream << "color " << color.x << " " << color.y << " " << color.z << "\n\n";

    return outStream.str();
}
