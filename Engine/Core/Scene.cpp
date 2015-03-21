#include "Scene.hpp"
#include "CameraComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"

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
    ae3d::System::Print("mutsis");
    
    if (mainCamera == nullptr)
    {
        ae3d::System::Print("mainCamera is null.");
        return;
    }
    
    CameraComponent* camera = mainCamera->GetComponent<CameraComponent>();

    Vec3 color = camera->GetClearColor();
    GfxDevice::SetClearColor( color.x, color.y, color.z );
    GfxDevice::ClearScreen( GfxDevice::ClearFlags::Color | GfxDevice::ClearFlags::Depth );
    ae3d::System::Print("Cleared screen");
    
    for (auto gameObject : gameObjects)
    {
        if (gameObject->GetComponent<SpriteRendererComponent>())
        {
            // set sprite shader.
            // upload uniforms.
            // draw quad.
        }
    }
}

void ae3d::Scene::Update()
{

}
