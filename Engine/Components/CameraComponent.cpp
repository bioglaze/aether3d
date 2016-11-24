#include "CameraComponent.hpp"
#include <vector>
#include <locale>
#include <sstream>

std::vector< ae3d::CameraComponent > cameraComponents;
unsigned nextFreeCameraComponent = 0;

unsigned ae3d::CameraComponent::New()
{
    if (nextFreeCameraComponent == cameraComponents.size())
    {
        cameraComponents.resize( cameraComponents.size() + 10 );
    }

    return nextFreeCameraComponent++;
}

ae3d::CameraComponent* ae3d::CameraComponent::Get( unsigned index )
{
    return &cameraComponents[ index ];
}

ae3d::Vec3 ae3d::CameraComponent::GetScreenPoint( const ae3d::Vec3 &worldPoint, float viewWidth, float viewHeight ) const
{
    Matrix44 viewProjection;
    Matrix44::Multiply( viewMatrix, projectionMatrix, viewProjection );
    Vec4 pos( worldPoint );
    Matrix44::TransformPoint( pos, viewProjection, &pos );
    pos.x /= pos.w;
    pos.y /= pos.w;
    
    const float halfWidth = viewWidth * 0.5f;
    const float halfHeight = viewHeight * 0.5f;
    
    return Vec3( pos.x * halfWidth + halfWidth, -pos.y * halfHeight + halfHeight, pos.z );
}

void ae3d::CameraComponent::SetProjection( float left, float right, float bottom, float top, float aNear, float aFar )
{
    orthoParams.left = left;
    orthoParams.right = right;
    orthoParams.top = top;
    orthoParams.down = bottom;
    nearp = aNear;
    farp = aFar;
    projectionMatrix.MakeProjection( orthoParams.left, orthoParams.right, orthoParams.down, orthoParams.top, nearp, farp );
}

void ae3d::CameraComponent::SetProjection( float aFovDegrees, float aAspect, float aNear, float aFar )
{
    nearp = aNear;
    farp = aFar;
    fovDegrees = aFovDegrees;
    aspect = aAspect;
    projectionMatrix.MakeProjection( fovDegrees, aspect, aNear, aFar );
}

void ae3d::CameraComponent::SetProjection( const Matrix44& proj )
{
    projectionMatrix = proj;
}

void ae3d::CameraComponent::SetClearColor( const Vec3& color )
{
    clearColor = color;
}

void ae3d::CameraComponent::SetTargetTexture( ae3d::RenderTexture* renderTexture )
{
    targetTexture = renderTexture;
}

std::string ae3d::CameraComponent::GetSerialized() const
{
    std::stringstream outStream;
    std::locale c_locale( "C" );
    outStream.imbue( c_locale );

    outStream << "camera\n";

    outStream << "ortho " << orthoParams.left << " " << orthoParams.right << " " << orthoParams.top << " " << orthoParams.down <<
    " " << nearp << " " << farp << "\n";

    outStream << "projection ";
    
    if (projectionType == ProjectionType::Perspective)
    {
        outStream << "perspective\n";
    }
    else
    {
        outStream << "orthographic\n";
    }
    
    outStream << "persp " << fovDegrees << " " << aspect << " " << nearp << " " << farp << "\n";
    outStream << "layermask " << layerMask << "\n";
    outStream << "order " << renderOrder << "\n";
    outStream << "clearcolor " << clearColor.x << " " << clearColor.y << " " << clearColor.z << "\n\n";

    return outStream.str();
}
