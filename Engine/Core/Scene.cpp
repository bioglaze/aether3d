#include "Scene.hpp"
#include <list>
#include "AudioSourceComponent.hpp"
#include "CameraComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "TextRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "GfxDevice.hpp"
#include "Shader.hpp"
#include "GameObject.hpp"
#include "System.hpp"

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

    if (camera && mainCamera == nullptr)
    {
        mainCamera = gameObject;
    }
}

void ae3d::Scene::Remove( GameObject* gameObject )
{
    for (std::size_t i = 0; i < gameObjects.size(); ++i)
    {
        if (gameObject == gameObjects[ i ])
        {
            gameObjects.erase( std::begin( gameObjects ) + i );
            return;
        }
    }
}

void ae3d::Scene::Render()
{
    if (mainCamera == nullptr || (mainCamera != nullptr && mainCamera->GetComponent<CameraComponent>() == nullptr))
    {
        return;
    }
    
    std::list< CameraComponent* > rtCameras;
    
    for (auto gameObject : gameObjects)
    {
        if (gameObject == nullptr)
        {
            continue;
        }

        auto cameraComponent = gameObject->GetComponent<CameraComponent>();
        
        if (cameraComponent && cameraComponent->GetTargetTexture())
        {
            rtCameras.push_back( cameraComponent );
        }
    }
    
    for (auto rtCamera : rtCameras)
    {
        RenderWithCamera( rtCamera );
    }
    
    CameraComponent* camera = mainCamera->GetComponent<CameraComponent>();

    RenderWithCamera( camera );
}

void ae3d::Scene::RenderWithCamera( CameraComponent* camera )
{
    System::Assert( camera != nullptr, "camera is null!" );
    
    GfxDevice::SetRenderTarget( camera->GetTargetTexture() );
    
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
        
        auto transform = gameObject->GetComponent<TransformComponent>();
        
        // TODO: Watch for this duplication of logic.
        
        auto spriteRenderer = gameObject->GetComponent<SpriteRendererComponent>();
        
        if (spriteRenderer)
        {
            Matrix44 projectionModel;
            Matrix44::Multiply( transform ? transform->GetLocalMatrix() : Matrix44::identity, camera->GetProjection(), projectionModel );
            spriteRenderer->Render( projectionModel.m );
        }
        
        auto textRenderer = gameObject->GetComponent<TextRendererComponent>();
        
        if (textRenderer)
        {
            Matrix44 projectionModel;
            Matrix44::Multiply( transform ? transform->GetLocalMatrix() : Matrix44::identity, camera->GetProjection(), projectionModel );
            textRenderer->Render( projectionModel.m );
        }
    }
    
    GfxDevice::ErrorCheck( "Scene render end" );
}

std::string ae3d::Scene::GetSerialized() const
{
    std::string outSerialized;

    for (auto gameObject : gameObjects)
    {
        if (gameObject == nullptr)
        {
            continue;
        }

        outSerialized += gameObject->GetSerialized();
        
        // TODO: Try to DRY.
        if (gameObject->GetComponent<TransformComponent>())
        {
            outSerialized += gameObject->GetComponent<TransformComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<CameraComponent>())
        {
            outSerialized += gameObject->GetComponent<CameraComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<TextRendererComponent>())
        {
            outSerialized += gameObject->GetComponent<TextRendererComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<AudioSourceComponent>())
        {
            outSerialized += gameObject->GetComponent<AudioSourceComponent>()->GetSerialized();
        }
    }
    return outSerialized;
}