#include "TransformComponent.hpp"
#include <vector>
#include "System.hpp"

std::vector< ae3d::TransformComponent > transformComponents;
int nextFreeTransformComponent = 0;

int ae3d::TransformComponent::New()
{
    if (nextFreeTransformComponent == transformComponents.size())
    {
        transformComponents.resize( transformComponents.size() + 10 );
    }

    return nextFreeTransformComponent++;
}

ae3d::TransformComponent* ae3d::TransformComponent::Get(int index)
{
    return &transformComponents[ index ];
}

void ae3d::TransformComponent::SetLocalPosition( const Vec3& localPos )
{
    localPosition = localPos;
    localMatrix = Matrix44::identity;
    localMatrix.Translate( localPosition );
}