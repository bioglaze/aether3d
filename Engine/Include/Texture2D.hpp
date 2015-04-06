#ifndef TEXTURE_2D_H
#define TEXTURE_2D_H

namespace ae3d
{
    namespace System
    {
        struct FileContentsData;
    }

    enum class TextureWrap
    {
        Repeat,
        Clamp
    };

    enum class TextureFilter
    {
        Nearest,
        Linear
    };

    /**
      2D texture.
    */
    class Texture2D
    {
    public:
        // \param textureData Texture data. File format must be png, tga, jpg, bmp or bmp.
        void Load( const System::FileContentsData& textureData, TextureWrap wrap, TextureFilter filter );
        
        // \return id.
        unsigned GetID() const { return id; }
        
        // \return Width in pixels.
        int GetWidth() const { return width; }

        // \return Width in pixels.
        int GetHeight() const { return height; }
        
        // \return Wrapping mode
        TextureWrap GetWrap() const { return wrap; }

        // \return Filtering mode
        TextureFilter GetFilter() const { return filter; }

        // \return True, if the texture does not contain an alpha channel.
        bool IsOpaque() const { return opaque; }

    private:
        void LoadDDS( const char* path );
        
        /**
          Loads texture from stb_image.c supported formats.

          \param textureData Texture data.
          */
        void LoadSTB( const System::FileContentsData& textureData );

        int width = 0;
        int height = 0;
        unsigned id = 0;
        TextureWrap wrap = TextureWrap::Repeat;
        TextureFilter filter = TextureFilter::Nearest;
        bool opaque = true;
    };
}
#endif
