#ifndef CREATESPRITERENDERERCOMMAND_HPP
#define CREATESPRITERENDERERCOMMAND_HPP

#include "Command.hpp"

namespace ae3d
{
    class GameObject;
}

class CreateSpriteRendererCommand : public CommandBase
{
public:
    explicit CreateSpriteRendererCommand( class SceneWidget* sceneWidget );
    void Execute() override;
    void Undo() override;

private:
    SceneWidget* sceneWidget = nullptr;
    ae3d::GameObject* gameObject = nullptr;
};

#endif
