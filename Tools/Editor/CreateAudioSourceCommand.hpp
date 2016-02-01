#ifndef CREATEAUDIOSOURCECOMMAND_HPP
#define CREATEAUDIOSOURCECOMMAND_HPP

#include "Command.hpp"

namespace ae3d
{
    class GameObject;
}

class CreateAudioSourceCommand : public CommandBase
{
public:
    explicit CreateAudioSourceCommand( class SceneWidget* sceneWidget );
    void Execute() override;
    void Undo() override;

private:
    SceneWidget* sceneWidget = nullptr;
    ae3d::GameObject* gameObject = nullptr;
};

#endif
