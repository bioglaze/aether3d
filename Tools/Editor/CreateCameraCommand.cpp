#include <list>
#include "CreateCameraCommand.hpp"
#include "CameraComponent.hpp"
#include "SceneWidget.hpp"
#include "System.hpp"

CreateCameraCommand::CreateCameraCommand( SceneWidget* aSceneWidget )
{
    sceneWidget = aSceneWidget;
}

void CreateCameraCommand::Execute()
{
    ae3d::System::Assert( sceneWidget != nullptr, "Create camera needs scene." );
    ae3d::System::Assert( !sceneWidget->selectedGameObjects.empty(), "no gameobjects selected." );

    gameObject = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() );

    if (!gameObject->GetComponent< ae3d::CameraComponent >())
    {
        gameObject->AddComponent< ae3d::CameraComponent >();
        gameObject->GetComponent< ae3d::CameraComponent >()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Perspective );
        gameObject->GetComponent< ae3d::CameraComponent >()->SetProjection( 45, 1, 1, 200 );
    }
}

void CreateCameraCommand::Undo()
{
    gameObject->RemoveComponent< ae3d::CameraComponent >();
}
