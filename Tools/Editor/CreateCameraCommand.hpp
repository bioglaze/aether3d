#ifndef CREATECAMERACOMMAND_HPP
#define CREATECAMERACOMMAND_HPP

#include "Command.hpp"

class SceneWidget;

class CreateCameraCommand : public CommandBase
{
public:
    explicit CreateCameraCommand( SceneWidget* sceneWidget );
    void Execute() override;
    void Undo() override;

private:
    SceneWidget* sceneWidget = nullptr;
};

#endif
