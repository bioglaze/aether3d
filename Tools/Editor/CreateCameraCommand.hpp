#ifndef CREATECAMERACOMMAND_HPP
#define CREATECAMERACOMMAND_HPP

#include "Command.hpp"

class SceneWidget;

namespace ae3d
{
    class GameObject;
}

class CreateCameraCommand : public CommandBase
{
public:
    explicit CreateCameraCommand( SceneWidget* sceneWidget );
    void Execute() override;
    void Undo() override;

private:
    SceneWidget* sceneWidget = nullptr;
    ae3d::GameObject* gameObject = nullptr;
};

#endif
