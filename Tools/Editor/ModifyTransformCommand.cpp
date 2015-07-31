#include "ModifyTransformCommand.hpp"
#include "SceneWidget.hpp"
#include "TransformComponent.hpp"

ModifyTransformCommand::ModifyTransformCommand( SceneWidget* aSceneWidget, const ae3d::Vec3& newPosition,
                                                const ae3d::Quaternion& newRotation, float newScale )
{
    sceneWidget = aSceneWidget;
    position = newPosition;
    rotation = newRotation;
    scale = newScale;
}

void ModifyTransformCommand::Execute()
{
    for (auto index : sceneWidget->selectedGameObjects)
    {
        auto transform = sceneWidget->GetGameObject( index )->GetComponent< ae3d::TransformComponent >();

        if (transform)
        {
            oldPosition = transform->GetLocalPosition();
            oldRotation = transform->GetLocalRotation();
            oldScale = transform->GetLocalScale();

            transform->SetLocalPosition( position );
            transform->SetLocalRotation( rotation );
            transform->SetLocalScale( scale );
        }
    }
}

void ModifyTransformCommand::Undo()
{
    for (auto index : sceneWidget->selectedGameObjects)
    {
        auto transform = sceneWidget->GetGameObject( index )->GetComponent< ae3d::TransformComponent >();

        if (transform)
        {
            transform->SetLocalPosition( oldPosition );
            transform->SetLocalRotation( oldRotation );
            transform->SetLocalScale( oldScale );
        }
    }
}
