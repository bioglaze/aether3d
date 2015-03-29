#ifndef SPRITE_RENDERER_COMPONENT_H
#define SPRITE_RENDERER_COMPONENT_H

#include <type_traits>

namespace ae3d
{
    class Texture2D;
    struct Vec3;
    
    /**
      Container for sprites.
     */
    class SpriteRendererComponent
    {
    public:
        // \return Component's type code. Must be unique for each component type.
        static int Type() { return 1; }

        // \return Component handle that uniquely identifies the instance.
        static int New();

        // \return Component at index or null if index is invalid.
        static SpriteRendererComponent* Get(int index);
        
        SpriteRendererComponent();

        // Removes all textures that were added using SetTexture.
        void Clear();
        
        /**
          Adds a texture to be rendered. The same texture can be added multiple
          times.
         
          \param texture Texture.
          \param position Position relative to the component's transform.

          // TODO [2015-03-28]: Set pivot and scale, maybe also rotation.
         */
        void SetTexture( Texture2D* texture, const Vec3& position );

#pragma message("Remove from public API")
        void Render( const float* projectionMatrix );
        
    private:
        struct Impl;
        Impl& m() { return reinterpret_cast<Impl&>(_storage); }
        Impl const& m() const { return reinterpret_cast<Impl const&>(_storage); }
        
        static const size_t StorageSize = 384;
        static const size_t StorageAlign = 16;
        
        std::aligned_storage<StorageSize, StorageAlign>::type _storage;
    };
}
#endif
