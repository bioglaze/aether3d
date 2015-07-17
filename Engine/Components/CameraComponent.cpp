#include "CameraComponent.hpp"
#include <vector>
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

void ae3d::CameraComponent::SetProjection( float left, float right, float bottom, float top, float aNear, float aFar )
{
    orthoParams.left = left;
    orthoParams.right = right;
    orthoParams.top = top;
    orthoParams.down = bottom;
    nearp = aNear;
    farp = aFar;
    isOrthographic = true;
    projectionMatrix.MakeProjection( left, right, bottom, top, nearp, farp );
}

void ae3d::CameraComponent::SetProjection( float aFovDegrees, float aAspect, float aNear, float aFar )
{
    nearp = aNear;
    farp = aFar;
    fovDegrees = aFovDegrees;
    aspect = aAspect;
    isOrthographic = false;
    projectionMatrix.MakeProjection( fovDegrees, aspect, aNear, aFar );
}

void ae3d::CameraComponent::SetClearColor( const Vec3& color )
{
    clearColor = color;
}

void ae3d::CameraComponent::SetTargetTexture( ae3d::RenderTexture2D* renderTexture2D )
{
    targetTexture = renderTexture2D;
}

void ae3d::CameraComponent::SetClearFlag( ClearFlag aClearFlag )
{
    clearFlag = aClearFlag;
}

std::string ae3d::CameraComponent::GetSerialized() const
{
    std::stringstream outStream;
    outStream << "camera\n";

    if (isOrthographic)
    {
        outStream << "ortho " << orthoParams.left << " " << orthoParams.right << " " << orthoParams.top << " " << orthoParams.down <<
    " " << nearp << " " << farp << "\n";
    }
    else
    {
        outStream << "persp " << fovDegrees << " " << aspect << " " << nearp << " " << farp << "\n";
    }
    
    outStream << "clearcolor" << clearColor.x << " " << clearColor.y << " " << clearColor.z << "\n\n";
    
    return outStream.str();
}
