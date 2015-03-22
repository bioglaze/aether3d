#include "CameraComponent.hpp"
#include "Vec3.hpp"

ae3d::CameraComponent ae3d::CameraComponent::cameraComponents[100];

int ae3d::CameraComponent::New()
{
    return 0;
}

ae3d::CameraComponent* ae3d::CameraComponent::Get(int index)
{
    return &cameraComponents[index];
}

void ae3d::CameraComponent::SetProjection( int left, int right, int bottom, int top, float near, float far )
{
    projectionMatrix.MakeProjection( left, right, bottom, top, near, far );
}

void ae3d::CameraComponent::SetClearColor(const Vec3& color)
{
    clearColor = color;
}

