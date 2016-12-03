#include "SpotLightComponent.hpp"
#include <locale>
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
        shadowMap.Create2D( mapSize, mapSize, RenderTexture::DataType::Float, TextureWrap::Clamp, TextureFilter::Linear );
    }
}

std::string ae3d::SpotLightComponent::GetSerialized() const
{
    std::stringstream outStream;
    std::locale c_locale( "C" );
    outStream.imbue( c_locale );

    outStream << "spotlight\n";
    outStream << "shadow " << (castsShadow ? 1 : 0) << "\n";
    outStream << "coneangle " << coneAngle << "\n\n";
    outStream << "color " << color.x << " " << color.y << " " << color.z << "\n\n";

    return outStream.str();
}
