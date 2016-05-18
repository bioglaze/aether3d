#ifndef CREATELIGHTCOMMAND_HPP
#define CREATELIGHTCOMMAND_HPP

#include "Command.hpp"

namespace ae3d
{
    class Scene;
    class GameObject;
}

class SceneWidget;

class CreateLightCommand : public CommandBase
{
public:
    enum class Type { Directional, Spot, Point };

    explicit CreateLightCommand( SceneWidget* sceneWidget, Type type );
    void Execute() override;
    void Undo() override;

private:
    SceneWidget* sceneWidget = nullptr;
    ae3d::GameObject* go = nullptr;
    Type type = Type::Directional;
};

#endif
