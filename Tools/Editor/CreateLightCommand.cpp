#include "CreateLightCommand.hpp"
#include "DirectionalLightComponent.hpp"
#include "PointLightComponent.hpp"
#include "SpotLightComponent.hpp"
#include "System.hpp"
#include "Scene.hpp"
#include "SceneWidget.hpp"

CreateLightCommand::CreateLightCommand( SceneWidget* aSceneWidget, Type aType )
{
    ae3d::System::Assert( aSceneWidget != nullptr, "Create light constructor needs sceneWidget" );

    sceneWidget = aSceneWidget;
    type = aType;
}

void CreateLightCommand::Execute()
{
    ae3d::System::Assert( sceneWidget != nullptr, "Create light execute needs sceneWidget" );
    ae3d::System::Assert( !sceneWidget->selectedGameObjects.empty(), "Create light needs selection" );

    go = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() );

    if (type == Type::Directional && !go->GetComponent< ae3d::DirectionalLightComponent >())
    {
        go->AddComponent< ae3d::DirectionalLightComponent >();
    }
    else if (type == Type::Spot && !go->GetComponent< ae3d::SpotLightComponent >())
    {
        go->AddComponent< ae3d::SpotLightComponent >();
    }
    else if (type == Type::Point && !go->GetComponent< ae3d::PointLightComponent >())
    {
        go->AddComponent< ae3d::PointLightComponent >();
    }
}

void CreateLightCommand::Undo()
{
    ae3d::System::Assert( sceneWidget != nullptr, "Create light undo needs sceneWidget" );
    ae3d::System::Assert( sceneWidget->GetScene() != nullptr, "Create light undo needs scene" );
    ae3d::System::Assert( go != nullptr, "Create light undo need game object." );

    if (type == Type::Directional)
    {
        go->RemoveComponent< ae3d::DirectionalLightComponent >();
    }
    else if (type == Type::Spot)
    {
        go->RemoveComponent< ae3d::SpotLightComponent >();
    }
    else if (type == Type::Point)
    {
        go->RemoveComponent< ae3d::PointLightComponent >();
    }
}
