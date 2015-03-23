#include "CameraComponent.hpp"

const int CameraComponentMax = 100;
ae3d::CameraComponent ae3d::CameraComponent::cameraComponents[CameraComponentMax];
int nextFreeCameraComponent = 0;

int ae3d::CameraComponent::New()
{
    return nextFreeCameraComponent++;
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

