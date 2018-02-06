#include "TransformComponent.hpp"
#include <locale>
#include <vector>
#include <string>
#include <sstream>
#include "Matrix.hpp"
#include "System.hpp"

namespace
{
    bool IsAlmost( float f1, float f2 )
    {
        return std::abs( f1 - f2 ) < 0.0001f;
    }

    std::vector< ae3d::TransformComponent > transformComponents;
    unsigned nextFreeTransformComponent = 0;
}

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
    System::Assert( index < static_cast< unsigned >( transformComponents.size() ), "invalid transform index" );
    return &transformComponents[ index ];
}

ae3d::TransformComponent* ae3d::TransformComponent::GetParent() const
{
    System::Assert( parent < static_cast< int >( transformComponents.size() ), "invalid parent transform index" );
    return parent == -1 ? nullptr : &transformComponents[ parent ];
}

void ae3d::TransformComponent::LookAt( const Vec3& aLocalPosition, const Vec3& center, const Vec3& up )
{
    Matrix44 lookAt;
    lookAt.MakeLookAt( aLocalPosition, center, up );
    localRotation.FromMatrix( lookAt );
    localPosition = aLocalPosition;
}

void ae3d::TransformComponent::MoveForward( float amount )
{
    if (!IsAlmost( amount, 0 ))
    {
        localPosition += localRotation * Vec3( 0, 0, amount );
    }
}

void ae3d::TransformComponent::MoveRight( float amount )
{
    if (!IsAlmost( amount, 0 ))
    {
        localPosition += localRotation * Vec3( amount, 0, 0 );
    }
}

void ae3d::TransformComponent::MoveUp( float amount )
{
    localPosition.y += amount;
}

void ae3d::TransformComponent::OffsetRotate( const Vec3& axis, float angleDeg )
{
    Quaternion rot;
    rot.FromAxisAngle( axis, angleDeg );

    Quaternion newRotation;

    if (IsAlmost( axis.y, 0 ))
    {
        newRotation = localRotation * rot;
    }
    else
    {
        newRotation = rot * localRotation;
    }

    newRotation.Normalize();

    if ((IsAlmost( axis.x, 1 ) || IsAlmost( axis.x, -1 )) && IsAlmost( axis.y, 0 ) && IsAlmost( axis.z, 0 ) &&
        newRotation.FindTwist( Vec3( 1.0f, 0.0f, 0.0f ) ) > 0.9999f)
    {
        return;
    }

    localRotation = newRotation;
}

void ae3d::TransformComponent::UpdateLocalMatrices()
{
    for (unsigned componentIndex = 0; componentIndex < nextFreeTransformComponent; ++componentIndex)
    {
        transformComponents[ componentIndex ].SolveLocalMatrix();
    }

    for (unsigned componentIndex = 0; componentIndex < nextFreeTransformComponent; ++componentIndex)
    {
        int parent = transformComponents[ componentIndex ].parent;

        Matrix44 transform = transformComponents[ componentIndex ].localMatrix;

        Quaternion worldRotation = transformComponents[ componentIndex ].localRotation;
        
        while (parent != -1)
        {
            Matrix44::Multiply( transform, transformComponents[ parent ].GetLocalMatrix(), transform );
            worldRotation = worldRotation * transformComponents[ parent ].localRotation;
            parent = transformComponents[ parent ].parent;
        }

        transformComponents[ componentIndex ].localToWorldMatrix = transform;
        Matrix44::TransformPoint( Vec3( 0, 0, 0 ), transform, &transformComponents[ componentIndex ].globalPosition );
        transformComponents[ componentIndex ].globalRotation = worldRotation;
    }
}

const ae3d::Matrix44& ae3d::TransformComponent::GetLocalMatrix()
{
    return localMatrix;
}

void ae3d::TransformComponent::SetLocalPosition( const Vec3& localPos )
{
    localPosition = localPos;
}

void ae3d::TransformComponent::SetLocalRotation( const Quaternion& localRot )
{
    localRotation = localRot;
}

void ae3d::TransformComponent::SetLocalScale( float aLocalScale )
{
    localScale = aLocalScale;
}

void ae3d::TransformComponent::SolveLocalMatrix()
{
    localRotation.GetMatrix( localMatrix );
    localMatrix.Scale( localScale, localScale, localScale );
    localMatrix.SetTranslation( localPosition );
}

void ae3d::TransformComponent::SetVrView( const Matrix44& view )
{
    (void)view;
    
#if defined( AE3D_OPENVR )
    hmdView = view;
#endif
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
        
        testComponent = testComponent->parent == -1 ? nullptr : &transformComponents[ testComponent->parent ];
    }

    for (std::size_t componentIndex = 0; componentIndex < transformComponents.size(); ++componentIndex)
    {
        if (&transformComponents[ componentIndex ] == aParent)
        {
            parent = static_cast< int >( componentIndex );
            return;
        }
    }
}

std::string GetSerialized( ae3d::TransformComponent* component )
{
    std::stringstream outStream;
    std::locale c_locale( "C" );
    outStream.imbue( c_locale );

    auto localPosition = component->GetLocalPosition();
    auto localRotation = component->GetLocalRotation();
    
    outStream << "transform\nposition " << localPosition.x << " " << localPosition.y << " " << localPosition.z << "\nrotation ";
    outStream << localRotation.x << " " << localRotation.y << " " << localRotation.z << " " << localRotation.w << "\nscale " << component->GetLocalScale() << "\n";
    outStream << "enabled " << component->IsEnabled() << "\n\n";
    
    return outStream.str();
}

ae3d::Vec3 ae3d::TransformComponent::GetViewDirection() const
{
    ae3d::Matrix44 view;
    GetWorldRotation().GetMatrix( view );
    Matrix44 translation;
    translation.SetTranslation( -globalPosition );
    Matrix44::Multiply( translation, view, view );

    return Vec3( view.m[ 2 ], view.m[ 6 ], view.m[ 10 ] ).Normalized();
}

