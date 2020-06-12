// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "GameObject.hpp"
#include "AudioSourceComponent.hpp"
#include "CameraComponent.hpp"
#include "DirectionalLightComponent.hpp"
#include "MeshRendererComponent.hpp"
#include "PointLightComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "SpotLightComponent.hpp"
#include "TransformComponent.hpp"
#include "TextRendererComponent.hpp"

using namespace ae3d;

unsigned ae3d::GameObject::GetNextComponentIndex()
{
    return nextFreeComponentIndex >= MaxComponents ? InvalidComponentIndex : nextFreeComponentIndex++;
}

ae3d::GameObject::GameObject( const GameObject& other )
{
    *this = other;
}

GameObject& ae3d::GameObject::operator=( const GameObject& go )
{
    name = go.name;
    
    for (unsigned i = 0; i < MaxComponents; ++i)
    {
        components[ i ].type = -1;
        components[ i ].handle = InvalidComponentIndex;
    }

    if (go.GetComponent< TransformComponent >())
    {
        AddComponent< TransformComponent >();
        *GetComponent< TransformComponent >() = *go.GetComponent< TransformComponent >();
        GetComponent< TransformComponent >()->gameObject = this;
    }

    if (go.GetComponent< MeshRendererComponent >())
    {
        AddComponent< MeshRendererComponent >();
        *GetComponent< MeshRendererComponent >() = *go.GetComponent< MeshRendererComponent >();
        GetComponent< MeshRendererComponent >()->gameObject = this;
    }

    if (go.GetComponent< CameraComponent >())
    {
        AddComponent< CameraComponent >();
        *GetComponent< CameraComponent >() = *go.GetComponent< CameraComponent >();
        GetComponent< CameraComponent >()->gameObject = this;
    }

    if (go.GetComponent< DirectionalLightComponent >())
    {
        AddComponent< DirectionalLightComponent >();
        *GetComponent< DirectionalLightComponent >() = *go.GetComponent< DirectionalLightComponent >();
        GetComponent< DirectionalLightComponent >()->gameObject = this;
    }

    if (go.GetComponent< AudioSourceComponent >())
    {
        AddComponent< AudioSourceComponent >();
        *GetComponent< AudioSourceComponent >() = *go.GetComponent< AudioSourceComponent >();
        GetComponent< AudioSourceComponent >()->gameObject = this;
    }

    if (go.GetComponent< SpriteRendererComponent >())
    {
        AddComponent< SpriteRendererComponent >();
        *GetComponent< SpriteRendererComponent >() = *go.GetComponent< SpriteRendererComponent >();
        GetComponent< SpriteRendererComponent >()->gameObject = this;
    }

    if (go.GetComponent< TextRendererComponent >())
    {
        AddComponent< TextRendererComponent >();
        *GetComponent< TextRendererComponent >() = *go.GetComponent< TextRendererComponent >();
        GetComponent< TextRendererComponent >()->gameObject = this;
    }

    if (go.GetComponent< SpotLightComponent >())
    {
        AddComponent< SpotLightComponent >();
        *GetComponent< SpotLightComponent >() = *go.GetComponent< SpotLightComponent >();
        GetComponent< SpotLightComponent >()->gameObject = this;
    }

    if (go.GetComponent< PointLightComponent >())
    {
        AddComponent< PointLightComponent >();
        *GetComponent< PointLightComponent >() = *go.GetComponent< PointLightComponent >();
        GetComponent< PointLightComponent >()->gameObject = this;
    }

    return *this;
}

bool ae3d::GameObject::IsEnabled() const
{
    const TransformComponent* transform = GetComponent< TransformComponent >();

    while (transform)
    {
        if (transform->GetGameObject() && !transform->GetGameObject()->isEnabled)
        {
            return false;
        }
        
        transform = transform->GetParent();
    }
    
    return isEnabled;
}

std::string ae3d::GameObject::GetSerialized() const
{
    std::string serializedName = "gameobject\n";
    serializedName += "name ";
    serializedName += name.empty() ? "empty" : name;
    serializedName += "\n";
    serializedName += "layer ";
    serializedName += std::to_string( layer );
    serializedName += "\n";
    serializedName += "enabled ";
    serializedName += (isEnabled ? "1" : "0");
    serializedName += "\n\n";

    return serializedName;
}
