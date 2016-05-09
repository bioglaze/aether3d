#ifndef SPRITE_RENDERER_COMPONENT_H
#define SPRITE_RENDERER_COMPONENT_H

#include <type_traits>
#include <string>

namespace ae3d
{
    /// Renders sprites.
    class SpriteRendererComponent
    {
    public:
        /// Constructor.
        SpriteRendererComponent();

        /// \param other Other.
        SpriteRendererComponent( const SpriteRendererComponent& other );
        
        /// Destructor.
        ~SpriteRendererComponent();

        /// \param other Other.
        SpriteRendererComponent& operator=( const SpriteRendererComponent& other );

        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }

        /// \return Textual representation of component.
        std::string GetSerialized() const;

        /// Removes all textures that were added using SetTexture.
        void Clear();
        
        /**
          Adds a texture to be rendered. The same texture can be added multiple
          times.
         
          \param texture Texture.
          \param position Position relative to the component's transform.
          \param dimensionPixels Dimension in pixels.
          \param tintColor Tint color in range 0-1.

          // TODO [2015-03-28]: Set pivot, maybe also rotation.
         */
        void SetTexture( class TextureBase* texture, const struct Vec3& position, const Vec3& dimensionPixels, const struct Vec4& tintColor );

    private:
        friend class GameObject;
        friend class Scene;
        
        /* \return Component's type code. Must be unique for each component type. */
        static int Type() { return 1; }
        
        /* \return Component handle that uniquely identifies the instance. */
        static unsigned New();
        
        /* \return Component at index or null if index is invalid. */
        static SpriteRendererComponent* Get( unsigned index );

        /* \param projectionModelMatrix Projection and model matrix combined. */
        void Render( const float* projectionModelMatrix );
        
        struct Impl;
        Impl& m() { return reinterpret_cast<Impl&>(_storage); }
        Impl const& m() const { return reinterpret_cast<Impl const&>(_storage); }
        
        static const std::size_t StorageSize = 1384;
        static const std::size_t StorageAlign = 16;
        
        std::aligned_storage<StorageSize, StorageAlign>::type _storage = {};
        
        GameObject* gameObject = nullptr;
    };
}
#endif
