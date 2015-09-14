#include "DirectionalLightComponent.hpp"
#include <vector>

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
        shadowMap.Create2D( shadowMapSize, shadowMapSize, TextureWrap::Clamp, TextureFilter::Nearest );
    }
}
