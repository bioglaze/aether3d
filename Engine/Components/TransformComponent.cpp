#include "TransformComponent.hpp"

ae3d::TransformComponent ae3d::TransformComponent::transformComponents[100];
int nextFreeTransformComponent = 0;

int ae3d::TransformComponent::New()
{
    return nextFreeTransformComponent++;
}

ae3d::TransformComponent* ae3d::TransformComponent::Get(int index)
{
    return &transformComponents[ index ];
}

void ae3d::TransformComponent::SetLocalPosition( const Vec3& localPos )
{
    localPosition = localPos;
}