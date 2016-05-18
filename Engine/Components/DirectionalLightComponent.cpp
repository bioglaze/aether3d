#include "DirectionalLightComponent.hpp"
#include <vector>
#include <sstream>

std::vector< ae3d::DirectionalLightComponent > directionalLightComponents;
unsigned nextFreeDirectionalLightComponent = 0;

unsigned ae3d::DirectionalLightComponent::New()
{
    if (nextFreeDirectionalLightComponent == directionalLightComponents.size())
    {
        directionalLightComponents.resize( directionalLightComponents.size() + 10 );
    }

    return nextFreeDirectionalLightComponent++;
}

ae3d::DirectionalLightComponent* ae3d::DirectionalLightComponent::Get( unsigned index )
{
    return &directionalLightComponents[ index ];
}

void ae3d::DirectionalLightComponent::SetCastShadow( bool enable, int shadowMapSize )
{
    castsShadow = enable;
    const int mapSize = (shadowMapSize > 0 && shadowMapSize < 16385) ? shadowMapSize : 512;
    
    // TODO: create only if not already created with current size.
    if (castsShadow)
    {
        shadowMap.Create2D( mapSize, mapSize, RenderTexture::DataType::Float, TextureWrap::Clamp, TextureFilter::Linear );
    }
}

std::string ae3d::DirectionalLightComponent::GetSerialized() const
{
    std::stringstream outStream;
    outStream << "dirlight\n";
    outStream << "shadow " << (castsShadow ? 1 : 0) << "\n";
    return outStream.str();
}
