#include "PointLightComponent.hpp"
#include <locale>
#include <vector>
#include <sstream>

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
    
    // TODO: create only if not already created with current size.
    if (castsShadow)
    {
        shadowMap.CreateCube( mapSize, RenderTexture::DataType::R32G32, TextureWrap::Clamp, TextureFilter::Linear, "pointlight shadow" );
    }
}

std::string ae3d::PointLightComponent::GetSerialized() const
{
    std::stringstream outStream;
    std::locale c_locale( "C" );
    outStream.imbue( c_locale );

    outStream << "pointlight\n";
    outStream << "shadow " << (castsShadow ? 1 : 0) << "\n";
    outStream << "radius " << radius << "\n";
    outStream << "color " << color.x << " " << color.y << " " << color.z << "\n\n";

    return outStream.str();
}
