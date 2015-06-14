#include "CreateGoCommand.hpp"
#include <iostream>
#include "System.hpp"
#include "Scene.hpp"
#include "SceneWidget.hpp"

CreateGoCommand::CreateGoCommand( SceneWidget* aSceneWidget )
{
    ae3d::System::Assert( aSceneWidget != nullptr, "Create game object constructor needs sceneWidget" );

    sceneWidget = aSceneWidget;
}

void CreateGoCommand::Execute()
{
    ae3d::System::Assert( sceneWidget != nullptr, "Create game object execute needs sceneWidget" );

    createdGoIndex = sceneWidget->CreateGameObject();
}

void CreateGoCommand::Undo()
{
    ae3d::System::Assert( sceneWidget != nullptr, "Create game object undo needs sceneWidget" );
    ae3d::System::Assert( sceneWidget->GetScene() != nullptr, "Create game object undo needs scene" );

    sceneWidget->RemoveGameObject( createdGoIndex );
}
