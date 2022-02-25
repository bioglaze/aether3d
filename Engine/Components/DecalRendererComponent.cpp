#include "DecalRendererComponent.hpp"
#include "System.hpp"
#include <string>

ae3d::DecalRendererComponent::DecalRendererComponent()
{

}

static constexpr int MaxComponents = 30;

ae3d::DecalRendererComponent decalRendererComponents[ MaxComponents ];
unsigned nextFreeDecalRendererComponent = 0;

unsigned ae3d::DecalRendererComponent::New()
{
    if (nextFreeDecalRendererComponent == MaxComponents - 1)
    {
        return nextFreeDecalRendererComponent;
    }
    
    return nextFreeDecalRendererComponent++;
}

ae3d::DecalRendererComponent* ae3d::DecalRendererComponent::Get( unsigned index )
{
    return &decalRendererComponents[ index ];
}

std::string GetSerialized( ae3d::DecalRendererComponent* component )
{
    std::string outStr = "decalrenderer\n";
    outStr += "decalrenderer_enabled ";
    outStr += std::to_string( component->IsEnabled() ? 1 : 0 );
    return outStr;
}

