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
                         ae3d::CameraComponent::ProjectionType projectionType,
                         const ae3d::Vec4& orthoParams, const ae3d::Vec4& perspParams );
    void Execute() override;
    void Undo() override;

    ae3d::CameraComponent* camera = nullptr;

    ae3d::Vec4 orthoParams;
    ae3d::Vec4 perspParams;
    ae3d::CameraComponent::ClearFlag clearFlag;
    ae3d::CameraComponent::ProjectionType projectionType;
    ae3d::Vec3 clearColor;

    ae3d::Vec4 oldOrthoParams;
    ae3d::Vec4 oldPerspParams;
    ae3d::CameraComponent::ClearFlag oldClearFlag;
    ae3d::CameraComponent::ProjectionType oldProjectionType;
    ae3d::Vec3 oldClearColor;
};

#endif
