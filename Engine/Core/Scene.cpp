#include "Scene.hpp"
#include "CameraComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"
#include "Shader.hpp"

// Temporary solution for prototyping.
extern ae3d::BuiltinShaders builtinShaders;

ae3d::Scene::Scene()
{
    for (int i = 0; i < 10; ++i)
    {
        gameObjects[ i ] = nullptr;
    }
}

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
    if (mainCamera == nullptr)
    {
        ae3d::System::Print("mainCamera is null.");
        return;
    }
    
    CameraComponent* camera = mainCamera->GetComponent<CameraComponent>();

    Vec3 color = camera->GetClearColor();
    GfxDevice::SetClearColor( color.x, color.y, color.z );
    GfxDevice::ClearScreen( GfxDevice::ClearFlags::Color | GfxDevice::ClearFlags::Depth );
    
    for (auto gameObject : gameObjects)
    {
        if (gameObject && gameObject->GetComponent<SpriteRendererComponent>())
        {
            builtinShaders.spriteRendererShader.Use();
            builtinShaders.spriteRendererShader.SetMatrix( "_ProjectionMatrix", camera->GetProjection().m );
            // draw quad.
        }
    }
}

void ae3d::Scene::Update()
{
}
