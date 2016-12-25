#ifndef DELETEGAMEOBJECTCOMMAND_HPP
#define DELETEGAMEOBJECTCOMMAND_HPP

#include <vector>
#include <memory>
#include <list>
#include "Command.hpp"

namespace ae3d
{
    class GameObject;
}

class SceneWidget;

class DeleteGameObjectCommand : public CommandBase
{
public:
    explicit DeleteGameObjectCommand( class SceneWidget* sceneWidget );

    void Execute() override;
    void Undo() override;

private:
    std::list< ae3d::GameObject* > deletedGameObjects;
    std::vector< std::shared_ptr< ae3d::GameObject > > gameObjectsBeforeDelete;
    SceneWidget* sceneWidget = nullptr;
};

#endif
