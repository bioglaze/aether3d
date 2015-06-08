#include "CreateGoCommand.hpp"
#include <iostream>
#include "System.hpp"
#include "Scene.hpp"
#include "SceneWidget.hpp"

CreateGoCommand::CreateGoCommand( SceneWidget* aSceneWidget )
{
    ae3d::System::Assert( aSceneWidget != nullptr, "null pointer" );

    sceneWidget = aSceneWidget;
}

void CreateGoCommand::Execute()
{
    ae3d::System::Assert( sceneWidget != nullptr, "null pointer" );

    std::cout << "Creating new game object." << std::endl;
    createdGoIndex = sceneWidget->CreateGameObject();
}

void CreateGoCommand::Undo()
{
    ae3d::System::Assert( sceneWidget != nullptr, "null pointer" );
    ae3d::System::Assert( sceneWidget->GetScene() != nullptr, "null pointer" );

    std::cout << "UNDO Creating new game object." << std::endl;
    sceneWidget->GetScene()->Remove( sceneWidget->GetGameObject( createdGoIndex ) );
}
