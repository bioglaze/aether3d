#include "TransformComponent.hpp"
#include <vector>
#include <sstream>

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
    const TransformComponent* testComponent = parent;

    while (testComponent != nullptr)
    {
        if (testComponent->isDirty)
        {
            isDirty = true;
            break;
        }

        testComponent = testComponent->parent;
    }

    if (isDirty)
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
    const TransformComponent* testComponent = aParent;
    
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

std::string ae3d::TransformComponent::GetSerialized() const
{
    std::stringstream outStream;
    outStream << "transform\n" << localPosition.x << " " << localPosition.y << " " << localPosition.z << "\n";
    outStream << localRotation.x << " " << localRotation.y << " " << localRotation.z << " " << localRotation.w << "\n\n";
    
    return outStream.str();
}
