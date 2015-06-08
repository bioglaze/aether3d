#ifndef CREATEGOCOMMAND_HPP
#define CREATEGOCOMMAND_HPP

#include "Command.hpp"

namespace ae3d
{
    class Scene;
}

class SceneWidget;

class CreateGoCommand : public ICommand
{
public:
    CreateGoCommand( SceneWidget* aSceneWidget );
    void Execute();
    void Undo();

private:
    SceneWidget* sceneWidget = nullptr;
    int createdGoIndex = 0;
};

#endif
