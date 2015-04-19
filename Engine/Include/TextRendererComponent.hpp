#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <string>

namespace ae3d
{
    class Font;

    /* Contains text loaded from AngelCode BMFont: http://www.angelcode.com/products/bmfont/ */
    class TextRendererComponent
    {
      public:
        /* Constructor. */
        TextRendererComponent();
        
        /* Destructor. */
        ~TextRendererComponent();

        /*
           \param fontTex Font texture.
           \param metaData BMFont metadata in binary or text.
         */
        void SetFont( Font* font );
        
        /* \param text Text. Characters not in font are rendered empty. */
        void SetText( const char* text );

      private:
        friend class GameObject;
        friend class Scene;

        /* \return Component's type code. Must be unique for each component type. */
        static int Type() { return 4; }
        
        /* \return Component handle that uniquely identifies the instance. */
        static int New();
        
        /* \return Component at index or null if index is invalid. */
        static TextRendererComponent* Get( int index );

        /* \param projectionModelMatrix Projection and model matrix combined. */
        void Render( const float* projectionModelMatrix );

        struct Impl;
        Impl& m() { return reinterpret_cast<Impl&>(_storage); }
        Impl const& m() const { return reinterpret_cast<Impl const&>(_storage); }
        
        static const std::size_t StorageSize = 1384;
        static const std::size_t StorageAlign = 16;
        
        std::aligned_storage<StorageSize, StorageAlign>::type _storage;

        //std::string text;
        //Font* font = nullptr;
    };
}

#endif
