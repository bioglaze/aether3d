#include "DeleteGameObjectCommand.hpp"
#include "SceneWidget.hpp"
#include "Scene.hpp"
#include <iostream>

DeleteGameObjectCommand::DeleteGameObjectCommand( SceneWidget* aSceneWidget )
{
    sceneWidget = aSceneWidget;

    for (auto selectedIndex : sceneWidget->selectedGameObjects)
    {
        deletedGameObjects.push_front( sceneWidget->GetGameObject( selectedIndex ) );
    }

    gameObjectsBeforeDelete = sceneWidget->GetGameObjects();
}

void DeleteGameObjectCommand::Execute()
{
    for (auto goIt : deletedGameObjects)
    {
        sceneWidget->GetScene()->Remove( goIt );
    }

    std::vector< std::shared_ptr< ae3d::GameObject > > gameObjectsAfterDelete;

    auto sceneGos = sceneWidget->GetGameObjects();

    for (std::size_t go = 0; go < sceneGos.size(); ++go)
    {
        bool wasDeleted = false;

        for (auto goIt : deletedGameObjects)
        {
            if (goIt == sceneGos[ go ].get())
            {
                wasDeleted = true;
            }
        }

        if (!wasDeleted)
        {
            gameObjectsAfterDelete.push_back( sceneGos[ go ] );
        }
    }

    sceneWidget->SetGameObjects( gameObjectsAfterDelete );
}

void DeleteGameObjectCommand::Undo()
{
    for (auto goIt : deletedGameObjects)
    {
        sceneWidget->GetScene()->Add( goIt );
    }

    sceneWidget->SetGameObjects( gameObjectsBeforeDelete );
}
