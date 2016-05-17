#include "PointLightComponent.hpp"
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
        shadowMap.CreateCube( mapSize, RenderTexture::DataType::Float, TextureWrap::Clamp, TextureFilter::Linear );
    }
}

std::string ae3d::PointLightComponent::GetSerialized() const
{
    std::stringstream outStream;
    outStream << "pointlight\n";
    outStream << (castsShadow ? 1 : 0);
    return outStream.str();
}
