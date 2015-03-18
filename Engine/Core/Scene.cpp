#include "Scene.hpp"
#include "CameraComponent.hpp"
#include "GfxDevice.hpp"

void ae3d::Scene::Add( GameObject* gameObject )
{
    gameObjects[0] = gameObject;

    CameraComponent* camera = gameObject->GetComponent<CameraComponent>();

    if (camera && !mainCamera)
    {
        mainCamera = gameObject;
    }
}

void ae3d::Scene::Render()
{
    if (mainCamera)
    {
        CameraComponent* camera = mainCamera->GetComponent<CameraComponent>();

        Vec3 color = camera->GetClearColor();
        GfxDevice::SetClearColor( color.x, color.y, color.z );
        GfxDevice::ClearScreen( GfxDevice::ClearFlags::Color | GfxDevice::ClearFlags::Depth );
    }
}

void ae3d::Scene::Update()
{

}
