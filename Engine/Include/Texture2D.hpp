#ifndef TEXTURE_2D_H
#define TEXTURE_2D_H

#include "Vec3.hpp"

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
      2D immutable texture.
    */
    class Texture2D
    {
    public:
        // \param textureData Texture image data. File format must be dds, png, tga, jpg, bmp or bmp.
        // \param wrap Wrap mode.
        // \param filter Filter mode.
        void Load( const System::FileContentsData& textureData, TextureWrap wrap, TextureFilter filter );
        
        // \param atlasTextureData Atlas texture image data. File format must be dds, png, tga, jpg, bmp or bmp.
        // \param atlasMetaData Atlas metadata. Format is Ogre/CEGUI. Example atlas tool: Texture Packer.
        // \param textureName Name of the texture in atlas.
        // \param wrap Wrap mode.
        // \param filter Filter mode.
        void LoadFromAtlas( const System::FileContentsData& atlasTextureData, const System::FileContentsData& atlasMetaData, const char* textureName, TextureWrap wrap, TextureFilter filter );

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

        const Vec4& GetScaleOffset() const { return scaleOffset; }

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
        Vec4 scaleOffset{ 1, 1, 0, 0 };
    };
}
#endif
