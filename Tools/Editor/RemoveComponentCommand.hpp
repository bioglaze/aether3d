#ifndef REMOVECOMPONENTCOMMAND_HPP
#define REMOVECOMPONENTCOMMAND_HPP

#include "Command.hpp"

namespace ae3d
{
    class AudioSourceComponent;
    class CameraComponent;
    class MeshRendererComponent;
    class SpriteRendererComponent;
    class SpotLightComponent;
    class PointLightComponent;
    class DirectionalLightComponent;
}

class RemoveComponentCommand : public CommandBase
{
public:
    explicit RemoveComponentCommand( ae3d::AudioSourceComponent* audio );
    explicit RemoveComponentCommand( ae3d::CameraComponent* camera );
    explicit RemoveComponentCommand( ae3d::MeshRendererComponent* mesh );
    explicit RemoveComponentCommand( ae3d::SpriteRendererComponent* sprite );
    explicit RemoveComponentCommand( ae3d::SpotLightComponent* spotlight );
    explicit RemoveComponentCommand( ae3d::PointLightComponent* pointlight );
    explicit RemoveComponentCommand( ae3d::DirectionalLightComponent* dirlight );
    void Execute() override;
    void Undo() override;

private:
    ae3d::AudioSourceComponent* audio = nullptr;
    ae3d::CameraComponent* camera = nullptr;
    ae3d::MeshRendererComponent* mesh = nullptr;
    ae3d::SpriteRendererComponent* sprite = nullptr;
    ae3d::SpotLightComponent* spotlight = nullptr;
    ae3d::PointLightComponent* pointlight = nullptr;
    ae3d::DirectionalLightComponent* dirlight = nullptr;
};

#endif
