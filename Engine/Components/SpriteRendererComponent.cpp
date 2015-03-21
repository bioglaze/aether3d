#include "SpriteRendererComponent.hpp"

ae3d::SpriteRendererComponent ae3d::SpriteRendererComponent::spriteRendererComponents[100];

int ae3d::SpriteRendererComponent::New()
{
    return 0;
}

ae3d::SpriteRendererComponent* ae3d::SpriteRendererComponent::Get(int index)
{
    return &spriteRendererComponents[index];
}

void ae3d::SpriteRendererComponent::SetTexture(Texture2D* aTexture)
{
    texture = aTexture;
}
