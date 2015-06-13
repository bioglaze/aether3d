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

    sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() )->AddComponent< ae3d::CameraComponent >();
}

void CreateCameraCommand::Undo()
{
}
