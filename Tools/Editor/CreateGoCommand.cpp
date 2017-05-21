#include "CreateGoCommand.hpp"
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

    createdGo = sceneWidget->CreateGameObject();
}

void CreateGoCommand::Undo()
{
    ae3d::System::Assert( sceneWidget != nullptr, "Create game object undo needs sceneWidget" );
    ae3d::System::Assert( sceneWidget->GetScene() != nullptr, "Create game object undo needs scene" );
    ae3d::System::Assert( createdGo != nullptr, "Create game object undo need game object." );

    sceneWidget->GetScene()->Remove( createdGo );

    std::vector< std::shared_ptr< ae3d::GameObject > > gameObjectsAfterDelete;

    auto sceneGos = sceneWidget->GetGameObjects();

    for (std::size_t go = 0; go < sceneGos.size(); ++go)
    {
        bool wasDeleted = false;

        if (createdGo == sceneGos[ go ].get())
        {
            wasDeleted = true;
        }

        if (!wasDeleted)
        {
            gameObjectsAfterDelete.push_back( sceneGos[ go ] );
        }
    }

    sceneWidget->SetGameObjects( gameObjectsAfterDelete );
}
