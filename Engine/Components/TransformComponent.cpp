#include "TransformComponent.hpp"
#include <vector>
#include "System.hpp"

std::vector< ae3d::TransformComponent > transformComponents;
unsigned nextFreeTransformComponent = 0;

int ae3d::TransformComponent::New()
{
    if (nextFreeTransformComponent == transformComponents.size())
    {
        transformComponents.resize( transformComponents.size() + 10 );
    }

    return nextFreeTransformComponent++;
}

ae3d::TransformComponent* ae3d::TransformComponent::Get( unsigned index )
{
    return &transformComponents[ index ];
}

void ae3d::TransformComponent::SetLocalPosition( const Vec3& localPos )
{
    localPosition = localPos;
    localMatrix = Matrix44::identity;
    localMatrix.Translate( localPosition );
    localMatrix.Scale( localScale, localScale, localScale );
}

void ae3d::TransformComponent::SetLocalScale( float aLocalScale )
{
    localScale = aLocalScale;
    localMatrix = Matrix44::identity;
    localMatrix.Translate( localPosition );
    localMatrix.Scale( localScale, localScale, localScale );
}
