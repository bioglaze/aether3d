#ifndef MODIFYCAMERACOMMAND_HPP
#define MODIFYCAMERACOMMAND_HPP

#include "Command.hpp"
#include "CameraComponent.hpp"
#include "Vec3.hpp"

class ModifyCameraCommand : public CommandBase
{
public:
    ModifyCameraCommand( ae3d::CameraComponent* camera,
                         ae3d::CameraComponent::ClearFlag clearFlag,
                         const ae3d::Vec4& orthoParams, const ae3d::Vec4& perspParams );
    void Execute() override;
    void Undo() override;
    ae3d::CameraComponent* camera = nullptr;
    ae3d::Vec4 orthoParams;
    ae3d::Vec4 perspParams;
    ae3d::CameraComponent::ClearFlag clearFlag;
};

#endif
