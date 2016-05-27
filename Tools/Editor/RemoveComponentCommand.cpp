#include "RemoveComponentCommand.hpp"
#include "GameObject.hpp"
#include "System.hpp"
#include "SceneWidget.hpp"
#include "MeshRendererComponent.hpp"
#include "CameraComponent.hpp"
#include "DirectionalLightComponent.hpp"
#include "AudioSourceComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "TextRendererComponent.hpp"
#include "SpotLightComponent.hpp"
#include "PointLightComponent.hpp"

RemoveComponentCommand::RemoveComponentCommand( ae3d::AudioSourceComponent* audio )
{
    this->audio = audio;
}

RemoveComponentCommand::RemoveComponentCommand( ae3d::CameraComponent* camera )
{
    this->camera = camera;
}

RemoveComponentCommand::RemoveComponentCommand( ae3d::MeshRendererComponent* mesh )
{
    this->mesh = mesh;
}

RemoveComponentCommand::RemoveComponentCommand( ae3d::SpriteRendererComponent* sprite )
{
    this->sprite = sprite;
}

RemoveComponentCommand::RemoveComponentCommand( ae3d::SpotLightComponent* spotlight )
{
    this->spotlight = spotlight;
}

RemoveComponentCommand::RemoveComponentCommand( ae3d::PointLightComponent* pointlight )
{
    this->pointlight = pointlight;
}

RemoveComponentCommand::RemoveComponentCommand( ae3d::DirectionalLightComponent* dirlight )
{
    this->dirlight = dirlight;
}

void RemoveComponentCommand::Execute()
{
    ae3d::System::Print("removing component\n");
    if (audio) { audio->GetGameObject()->RemoveComponent< ae3d::AudioSourceComponent >(); }
    if (camera) { camera->GetGameObject()->RemoveComponent< ae3d::CameraComponent >(); }
    if (mesh) { mesh->GetGameObject()->RemoveComponent< ae3d::MeshRendererComponent >(); }
    if (sprite) { sprite->GetGameObject()->RemoveComponent< ae3d::SpriteRendererComponent >(); }
    if (spotlight) { spotlight->GetGameObject()->RemoveComponent< ae3d::SpotLightComponent >(); }
    if (pointlight) { pointlight->GetGameObject()->RemoveComponent< ae3d::PointLightComponent >(); }
    if (dirlight) { dirlight->GetGameObject()->RemoveComponent< ae3d::DirectionalLightComponent >(); }
}

void RemoveComponentCommand::Undo()
{
    ae3d::System::Print("undo removing component\n");
}
