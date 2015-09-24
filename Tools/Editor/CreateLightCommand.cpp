#include "CreateLightCommand.hpp"
#include "DirectionalLightComponent.hpp"
#include "System.hpp"
#include "Scene.hpp"
#include "SceneWidget.hpp"

CreateLightCommand::CreateLightCommand( SceneWidget* aSceneWidget )
{
    ae3d::System::Assert( aSceneWidget != nullptr, "Create light constructor needs sceneWidget" );

    sceneWidget = aSceneWidget;
}

void CreateLightCommand::Execute()
{
    ae3d::System::Assert( sceneWidget != nullptr, "Create light execute needs sceneWidget" );
    ae3d::System::Assert( !sceneWidget->selectedGameObjects.empty(), "Create light needs selection" );

    go = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() );
    go->AddComponent< ae3d::DirectionalLightComponent >();
}

void CreateLightCommand::Undo()
{
    ae3d::System::Assert( sceneWidget != nullptr, "Create light undo needs sceneWidget" );
    ae3d::System::Assert( sceneWidget->GetScene() != nullptr, "Create light undo needs scene" );
    ae3d::System::Assert( go != nullptr, "Create light undo need game object." );

    go->RemoveComponent< ae3d::DirectionalLightComponent >();
}
