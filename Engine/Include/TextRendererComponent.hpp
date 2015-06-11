#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <type_traits>
#include <string>

namespace ae3d
{
    class Font;
    struct Vec4;

    /// Contains text loaded from AngelCode BMFont: http://www.angelcode.com/products/bmfont/
    class TextRendererComponent
    {
      public:
        /// Shader type for rendering text.
        enum class ShaderType { Sprite, SDF };
        
        /// Constructor.
        TextRendererComponent();
        
        /// Destructor.
        ~TextRendererComponent();

        /// \param color Color in range 0-1.
        void SetColor( const Vec4& color );

        /// \param font Font.
        void SetFont( Font* font );
        
        /// \param text Text. Characters not in font are rendered empty.
        void SetText( const char* text );

        /// \param shaderType Shader type.
        void SetShader( ShaderType shaderType );

        /// \return Textual representation of component.
        std::string GetSerialized() const;

      private:
        friend class GameObject;
        friend class Scene;

        /** \return Component's type code. Must be unique for each component type. */
        static int Type() { return 4; }
        
        /** \return Component handle that uniquely identifies the instance. */
        static unsigned New();
        
        /** \return Component at index or null if index is invalid. */
        static TextRendererComponent* Get( unsigned index );

        /** \param projectionModelMatrix Projection and model matrix combined. */
        void Render( const float* projectionModelMatrix );

        struct Impl;
        Impl& m() { return reinterpret_cast<Impl&>(_storage); }
        Impl const& m() const { return reinterpret_cast<Impl const&>(_storage); }
        
        static const std::size_t StorageSize = 1384;
        static const std::size_t StorageAlign = 16;
        
        std::aligned_storage<StorageSize, StorageAlign>::type _storage;
    };
}

#endif
