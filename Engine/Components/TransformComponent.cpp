#include "TransformComponent.hpp"

ae3d::TransformComponent ae3d::TransformComponent::transformComponents[100];

int ae3d::TransformComponent::New()
{
    return 0;
}

ae3d::TransformComponent* ae3d::TransformComponent::Get(int index)
{
    return &transformComponents[ index ];
}

