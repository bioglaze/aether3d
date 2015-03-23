#ifndef SPRITE_RENDERER_COMPONENT_H
#define SPRITE_RENDERER_COMPONENT_H

namespace ae3d
{
    class Texture2D;

    class SpriteRendererComponent
    {
    public:
        // \return Component's type code. Must be unique for each component type.
        static int Type() { return 1; }

        // \return Component handle that uniquely identifies the instance.
        static int New();

        // \return Component at index or null if index is invalid.
        static SpriteRendererComponent* Get(int index);
        
        // \return Texture.
        const Texture2D* GetTexture() const { return texture; }
        // \param texture Texture.
        void SetTexture(Texture2D* texture);

    private:
        static ae3d::SpriteRendererComponent spriteRendererComponents[100];
        Texture2D* texture;
    };
}
#endif
