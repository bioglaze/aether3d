#include "ModifyCameraCommand.hpp"
#include "CameraComponent.hpp"
#include "System.hpp"

ModifyCameraCommand::ModifyCameraCommand( ae3d::CameraComponent* aCamera,
                                          ae3d::CameraComponent::ClearFlag aClearFlag,
                                          const ae3d::Vec4& aOrthoParams, const ae3d::Vec4& aPerspParams )
    : camera( aCamera )
    , clearFlag( aClearFlag )
    , orthoParams( orthoParams )
    , perspParams( perspParams )
{
    ae3d::System::Assert( camera, "Camera command needs camera!" );
}

void ModifyCameraCommand::Execute()
{
    ae3d::System::Print("Executing modify camera command\n");
    camera->SetClearFlag( clearFlag );
    // TODO: don't set camera projection type depending on setprojection call, instead provide setprojectiontype().
    camera->SetProjection( 0, orthoParams.x, 0, orthoParams.y, orthoParams.z, orthoParams.w );
    camera->SetProjection( perspParams.x, perspParams.y, perspParams.z, perspParams.w );
}

void ModifyCameraCommand::Undo()
{

}
