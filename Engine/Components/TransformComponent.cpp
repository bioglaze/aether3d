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

void ae3d::TransformComponent::LookAt( const Vec3& aLocalPosition, const Vec3& center, const Vec3& up )
{
    Matrix44 lookAt;
    lookAt.MakeLookAt( aLocalPosition, center, up );
    localRotation.FromMatrix( lookAt );
    localPosition = aLocalPosition;
    isDirty = true;
}

void ae3d::TransformComponent::MoveForward( float amount )
{
    if (amount > 0.00001f || amount < -0.00001f)
    {
        localPosition += localRotation * Vec3( 0, 0, amount );
        isDirty = true;
    }
}

void ae3d::TransformComponent::MoveRight( float amount )
{
    if (amount > 0.00001f || amount < -0.00001f)
    {
        localPosition += localRotation * Vec3( amount, 0, 0 );
        isDirty = true;
    }
}

void ae3d::TransformComponent::MoveUp( float amount )
{
    localPosition.y += amount;
    isDirty = true;
}

void ae3d::TransformComponent::OffsetRotate( const Vec3& axis, float angleDeg )
{
    Quaternion rot;
    rot.FromAxisAngle( axis, angleDeg );

    Quaternion newRotation;

    if (axis.y < 0.00001f && axis.y > -0.00001f )
    {
        newRotation = localRotation * rot;
    }
    else
    {
        newRotation = rot * localRotation;
    }

    newRotation.Normalize();

    if ((axis.x == 1.0f || axis.x == -1.0f) && axis.y == 0.0f && axis.z == 0.0f &&
        newRotation.FindTwist( Vec3( 1.0f, 0.0f, 0.0f ) ) > 0.9999f)
    {
        return;
    }

    localRotation = newRotation;
    isDirty = true;
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
