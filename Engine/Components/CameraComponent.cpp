#include "CameraComponent.hpp"
#include <vector>

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
    return &cameraComponents[index];
}

void ae3d::CameraComponent::SetProjection( float left, float right, float bottom, float top, float near, float far )
{
    projectionMatrix.MakeProjection( left, right, bottom, top, near, far );
}

void ae3d::CameraComponent::SetClearColor( const Vec3& color )
{
    clearColor = color;
}

