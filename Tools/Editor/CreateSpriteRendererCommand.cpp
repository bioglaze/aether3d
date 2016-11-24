#include "CreateSpriteRendererCommand.hpp"
#include "SpriteRendererComponent.hpp"
#include "GameObject.hpp"
#include "System.hpp"
#include "SceneWidget.hpp"

CreateSpriteRendererCommand::CreateSpriteRendererCommand( SceneWidget* aSceneWidget )
{
    sceneWidget = aSceneWidget;
}

void CreateSpriteRendererCommand::Execute()
{
    ae3d::System::Assert( sceneWidget != nullptr, "Create camera needs scene." );
    ae3d::System::Assert( !sceneWidget->selectedGameObjects.empty(), "no gameobjects selected." );

    gameObject = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() );

    if (!gameObject->GetComponent< ae3d::SpriteRendererComponent >())
    {
        gameObject->AddComponent< ae3d::SpriteRendererComponent >();
    }
}

void CreateSpriteRendererCommand::Undo()
{
    gameObject->RemoveComponent< ae3d::SpriteRendererComponent >();
}
