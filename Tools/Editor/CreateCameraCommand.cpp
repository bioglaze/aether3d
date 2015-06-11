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
    ae3d::System::Assert( sceneWidget != nullptr, "null pointer" );
    ae3d::System::Assert( sceneWidget->selectedGameObject != -1, "no gameobject selected." );

    sceneWidget->GetGameObject( sceneWidget->selectedGameObject )->AddComponent< ae3d::CameraComponent >();
}

void CreateCameraCommand::Undo()
{
}
