#include "TransformComponent.hpp"
#include <vector>
#include "System.hpp"

std::vector< ae3d::TransformComponent > transformComponents;
unsigned nextFreeTransformComponent = 0;

unsigned ae3d::TransformComponent::New()
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

const ae3d::Matrix44& ae3d::TransformComponent::GetLocalMatrix()
{
    if (isDirty || (parent && parent->isDirty))
    {
        SolveLocalMatrix();
        isDirty = false;
    }
    
    return localMatrix;
}

void ae3d::TransformComponent::SetLocalPosition( const Vec3& localPos )
{
    localPosition = localPos;
    isDirty = true;
}

void ae3d::TransformComponent::SetLocalRotation( const Quaternion& localRot )
{
    localRotation = localRot;
    isDirty = true;
}

void ae3d::TransformComponent::SetLocalScale( float aLocalScale )
{
    localScale = aLocalScale;
    isDirty = true;
}

void ae3d::TransformComponent::SolveLocalMatrix()
{
    localRotation.GetMatrix( localMatrix );
    localMatrix.Translate( localPosition );
    localMatrix.Scale( localScale, localScale, localScale );
    
    if (parent != nullptr)
    {
        Matrix44::Multiply( localMatrix, parent->GetLocalMatrix(), localMatrix );
    }
}

void ae3d::TransformComponent::SetParent( TransformComponent* aParent )
{
    TransformComponent* testComponent = aParent;
    
    // Disallows cycles.
    while (testComponent != nullptr)
    {
        if (testComponent == this)
        {
            return;
        }
        
        testComponent = testComponent->parent;
    }

    parent = aParent;
}
