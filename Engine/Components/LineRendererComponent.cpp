#include "LineRendererComponent.hpp"
#include "System.hpp"

ae3d::LineRendererComponent::LineRendererComponent()
{

}

static constexpr int MaxComponents = 30;

ae3d::LineRendererComponent lineRendererComponents[ MaxComponents ];
unsigned nextFreeLineRendererComponent = 0;

unsigned ae3d::LineRendererComponent::New()
{
    if (nextFreeLineRendererComponent == MaxComponents - 1)
    {
        return nextFreeLineRendererComponent;
    }
    
    return nextFreeLineRendererComponent++;
}

ae3d::LineRendererComponent* ae3d::LineRendererComponent::Get( unsigned index )
{
    return &lineRendererComponents[ index ];
}
