#pragma once

#include <type_traits>
#include <string>

namespace ae3d
{
    /// Sprite's path and dimensions.
    struct SpriteInfo
    {
        /// Path
        std::string path;
        /// X-coordinate in pixels
        float x;
        /// Y-coordinate in pixels
        float y;
        /// Width in pixels
        float width;
        /// Height in pixels
        float height;
        /// True, if the sprite has been loaded without errors
        bool valid;
    };

    /// Renders sprites. Can contain multiple sprites.
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

        /// \param enabled True if the component should be rendered, false otherwise.
        void SetEnabled( bool enabled ) { isEnabled = enabled; }

        /// \return Textual representation of component.
        std::string GetSerialized() const;

        /// Removes all textures that were added using SetTexture.
        void Clear();

        /// \return Sprite info for index.
        SpriteInfo GetSpriteInfo( int index ) const;

        /**
          Adds a texture to be rendered. The same texture can be added multiple
          times.
         
          \param texture Texture.
          \param position Position relative to the component's transform.
          \param dimensionPixels Dimension in pixels.
          \param tintColor Tint color in range 0-1.
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

        /* \param localToClip Transforms coordinates to clip space. */
        void Render( const float* localToClip );
        
        struct Impl;
        Impl& m() { return reinterpret_cast<Impl&>(_storage); }
        Impl const& m() const { return reinterpret_cast<Impl const&>(_storage); }
        
        static const std::size_t StorageSize = 1384;
        static const std::size_t StorageAlign = 16;
        
        std::aligned_storage<StorageSize, StorageAlign>::type _storage = {};
        
        GameObject* gameObject = nullptr;
        bool isEnabled = true;
    };
}
