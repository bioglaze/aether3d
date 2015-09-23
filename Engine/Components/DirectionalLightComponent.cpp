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

    if (castsShadow && shadowMapSize > 0)
    {
        shadowMap.Create2D( shadowMapSize, shadowMapSize, RenderTexture::DataType::Float, TextureWrap::Clamp, TextureFilter::Linear );
    }
}

std::string ae3d::DirectionalLightComponent::GetSerialized() const
{
    std::stringstream outStream;
    outStream << "dirlight\n";
    outStream << (castsShadow ? 1 : 0);
    return outStream.str();
}
