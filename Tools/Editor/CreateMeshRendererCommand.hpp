#ifndef CREATEMESHRENDERERCOMMAND_HPP
#define CREATEMESHRENDERERCOMMAND_HPP

#include "Command.hpp"

class SceneWidget;

namespace ae3d
{
    class GameObject;
}

class CreateMeshRendererCommand : public CommandBase
{
public:
    explicit CreateMeshRendererCommand( SceneWidget* sceneWidget );
    void Execute() override;
    void Undo() override;

private:
    SceneWidget* sceneWidget = nullptr;
    ae3d::GameObject* gameObject = nullptr;
};

#endif
