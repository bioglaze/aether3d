#include "CreateMeshRendererCommand.hpp"
#include "MeshRendererComponent.hpp"
#include "System.hpp"
#include "GameObject.hpp"
#include "SceneWidget.hpp"

CreateMeshRendererCommand::CreateMeshRendererCommand( SceneWidget* aSceneWidget )
{
    sceneWidget = aSceneWidget;
}

void CreateMeshRendererCommand::Execute()
{
    ae3d::System::Assert( sceneWidget != nullptr, "Create mesh renderer needs scene." );
    ae3d::System::Assert( !sceneWidget->selectedGameObjects.empty(), "no gameobjects selected." );

    gameObject = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() );
    gameObject->AddComponent< ae3d::MeshRendererComponent >();
}

void CreateMeshRendererCommand::Undo()
{
    gameObject->RemoveComponent< ae3d::MeshRendererComponent >();
}
