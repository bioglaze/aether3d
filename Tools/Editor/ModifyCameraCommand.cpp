#include "ModifyCameraCommand.hpp"
#include "CameraComponent.hpp"
#include "System.hpp"

ModifyCameraCommand::ModifyCameraCommand( ae3d::CameraComponent* aCamera,
                                          ae3d::CameraComponent::ClearFlag aClearFlag,
                                          ae3d::CameraComponent::ProjectionType aProjectionType,
                                          const ae3d::Vec4& aOrthoParams, const ae3d::Vec4& aPerspParams,
                                          const ae3d::Vec3& aClearColor )
    : camera( aCamera )
    , orthoParams( aOrthoParams )
    , perspParams( aPerspParams )
    , clearFlag( aClearFlag )
    , projectionType( aProjectionType )
    , clearColor( aClearColor )
{
    ae3d::System::Assert( camera, "Camera command needs camera!" );

    oldPerspParams.x = camera->GetFovDegrees();
    oldPerspParams.y = camera->GetAspect();
    oldPerspParams.z = camera->GetNear();
    oldPerspParams.w = camera->GetFar();

    oldOrthoParams.x = camera->GetRight();
    oldOrthoParams.y = camera->GetTop();
    oldOrthoParams.z = camera->GetNear();
    oldOrthoParams.w = camera->GetFar();

    oldClearFlag = camera->GetClearFlag();
    oldProjectionType = camera->GetProjectionType();
    oldClearColor = camera->GetClearColor();
}

void ModifyCameraCommand::Execute()
{
    camera->SetClearFlag( clearFlag );
    camera->SetClearColor( clearColor );
    camera->SetProjectionType( projectionType );

    if (projectionType == ae3d::CameraComponent::ProjectionType::Orthographic)
    {
        camera->SetProjection( perspParams.x, perspParams.y, perspParams.z, perspParams.w );
        camera->SetProjection( 0, orthoParams.x, 0, orthoParams.y, orthoParams.z, orthoParams.w );
    }
    else
    {
        camera->SetProjection( 0, orthoParams.x, 0, orthoParams.y, orthoParams.z, orthoParams.w );
        camera->SetProjection( perspParams.x, perspParams.y, perspParams.z, perspParams.w );
    }
}

void ModifyCameraCommand::Undo()
{
    ae3d::System::Assert( camera, "Undo needs camera" );

    camera->SetClearFlag( oldClearFlag );
    camera->SetClearColor( oldClearColor );
    camera->SetProjectionType( oldProjectionType );

    if (oldProjectionType == ae3d::CameraComponent::ProjectionType::Orthographic)
    {
        camera->SetProjection( oldPerspParams.x, oldPerspParams.y, oldPerspParams.z, oldPerspParams.w );
        camera->SetProjection( 0, oldOrthoParams.x, 0, oldOrthoParams.y, oldOrthoParams.z, oldOrthoParams.w );
    }
    else
    {
        camera->SetProjection( 0, oldOrthoParams.x, 0, oldOrthoParams.y, oldOrthoParams.z, oldOrthoParams.w );
        camera->SetProjection( oldPerspParams.x, oldPerspParams.y, oldPerspParams.z, oldPerspParams.w );
    }
}
