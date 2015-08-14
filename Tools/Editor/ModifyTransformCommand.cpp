#include "ModifyTransformCommand.hpp"
#include "SceneWidget.hpp"
#include "TransformComponent.hpp"

ModifyTransformCommand::ModifyTransformCommand( SceneWidget* aSceneWidget, const ae3d::Vec3& newPosition,
                                                const ae3d::Quaternion& newRotation, float newScale )
    : sceneWidget( aSceneWidget )
    , position( newPosition )
    , rotation( newRotation )
    , scale( newScale )
{
}

void ModifyTransformCommand::Execute()
{
    selectedGameObjects = sceneWidget->selectedGameObjects;
    oldPositions.resize( selectedGameObjects.size() );
    oldRotations.resize( selectedGameObjects.size() );
    oldScales.resize( selectedGameObjects.size() );

    int i = 0;

    for (auto index : sceneWidget->selectedGameObjects)
    {
        auto transform = sceneWidget->GetGameObject( index )->GetComponent< ae3d::TransformComponent >();

        if (transform)
        {
            oldPositions[ i ] = transform->GetLocalPosition();
            oldRotations[ i ] = transform->GetLocalRotation();
            oldScales[ i ] = transform->GetLocalScale();

            transform->SetLocalPosition( position );
            transform->SetLocalRotation( rotation );
            transform->SetLocalScale( scale );
        }

        ++i;
    }
}

void ModifyTransformCommand::Undo()
{
    int i = 0;

    for (auto index : selectedGameObjects)
    {
        auto transform = sceneWidget->GetGameObject( index )->GetComponent< ae3d::TransformComponent >();

        if (transform)
        {
            transform->SetLocalPosition( oldPositions[ i ] );
            transform->SetLocalRotation( oldRotations[ i ] );
            transform->SetLocalScale( oldScales[ i ] );
        }

        ++i;
    }
}
