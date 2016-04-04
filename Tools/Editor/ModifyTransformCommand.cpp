#include "ModifyTransformCommand.hpp"
#include "SceneWidget.hpp"
#include "TransformComponent.hpp"

ModifyTransformCommand::ModifyTransformCommand( int aGameObjectIndex, SceneWidget* aSceneWidget, const ae3d::Vec3& newPosition,
                                                const ae3d::Quaternion& newRotation, float newScale )
    : sceneWidget( aSceneWidget )
    , position( newPosition )
    , rotation( newRotation )
    , scale( newScale )
    , gameObjectIndex( aGameObjectIndex )
    , oldScale( 0 )
{
}

void ModifyTransformCommand::Execute()
{
    auto transform = sceneWidget->GetGameObject( gameObjectIndex )->GetComponent< ae3d::TransformComponent >();

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

void ModifyTransformCommand::Undo()
{
    auto go = sceneWidget->GetGameObject( gameObjectIndex );

    if (go != nullptr)
    {
        auto transform = go->GetComponent< ae3d::TransformComponent >();

        if (transform)
        {
            transform->SetLocalPosition( oldPosition );
            transform->SetLocalRotation( oldRotation );
            transform->SetLocalScale( oldScale );
        }
    }
}
