#include "DirectionalLightComponent.hpp"
#include <locale>
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
        shadowMap.Create2D( mapSize, mapSize, RenderTexture::DataType::R32G32, TextureWrap::Clamp, TextureFilter::Linear, "dirlight shadow" );
    }
}

std::string ae3d::DirectionalLightComponent::GetSerialized() const
{
    std::stringstream outStream;
    std::locale c_locale( "C" );
    outStream.imbue( c_locale );

    outStream << "dirlight\n";
    outStream << "color " << color.x << " " << color.y << " " << color.z << "\n";
    outStream << "enabled" << isEnabled << "\n";
    outStream << "shadow " << (castsShadow ? 1 : 0) << "\n\n";
    return outStream.str();
}
