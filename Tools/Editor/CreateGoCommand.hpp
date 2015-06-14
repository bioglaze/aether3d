#ifndef CREATEGOCOMMAND_HPP
#define CREATEGOCOMMAND_HPP

#include "Command.hpp"

namespace ae3d
{
    class Scene;
    class GameObject;
}

class SceneWidget;

class CreateGoCommand : public CommandBase
{
public:
    CreateGoCommand( SceneWidget* sceneWidget );
    void Execute() override;
    void Undo() override;

private:
    SceneWidget* sceneWidget = nullptr;
    ae3d::GameObject* createdGo = nullptr;
};

#endif
