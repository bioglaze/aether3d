#include "Scene.hpp"
#include "CameraComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"
#include "Shader.hpp"
#include "GameObject.hpp"

void ae3d::Scene::Add( GameObject* gameObject )
{
    for (const auto& go : gameObjects)
    {
        if (go == gameObject)
        {
            return;
        }
    }

    if (nextFreeGameObject == gameObjects.size())
    {
        gameObjects.resize( gameObjects.size() + 10 );
    }

    gameObjects[ nextFreeGameObject++ ] = gameObject;

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
        return;
    }
    
    CameraComponent* camera = mainCamera->GetComponent<CameraComponent>();

    Vec3 color = camera->GetClearColor();
    GfxDevice::SetClearColor( color.x, color.y, color.z );
    GfxDevice::ClearScreen( GfxDevice::ClearFlags::Color | GfxDevice::ClearFlags::Depth );
    GfxDevice::ResetFrameStatistics();
    
    for (auto gameObject : gameObjects)
    {
        if (gameObject == nullptr)
        {
            continue;
        }
        
        auto spriteRenderer = gameObject->GetComponent<SpriteRendererComponent>();
        auto transform = gameObject->GetComponent<TransformComponent>();
        
        if (gameObject && spriteRenderer)
        {
            Matrix44 projectionModel;
            Matrix44::Multiply( transform ? transform->GetLocalMatrix() : Matrix44::identity, camera->GetProjection(), projectionModel );
            spriteRenderer->Render( projectionModel.m );
        }
    }

    // TODO: Remove when text rendering is implemented.
    static int frame = 0;
    ++frame;
    if (frame % 60 == 0)
    {
        ae3d::System::Print("draw calls: %d", GfxDevice::GetDrawCalls());
    }

    GfxDevice::ErrorCheck( "Scene render end" );
}
