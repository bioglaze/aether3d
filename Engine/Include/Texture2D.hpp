#ifndef TEXTURE_2D_H
#define TEXTURE_2D_H

namespace ae3d
{
    namespace System
    {
        struct FileContentsData;
    }

    /**
      2D texture.
    */
    class Texture2D
    {
    public:
        // \param textureData Texture data. File format must be png, tga, jpg, bmp or bmp.
        void Load( const System::FileContentsData& textureData );
        // \return id.
        unsigned GetID() const { return id; }

    private:
#pragma message("TODO: Load from data and obey SRP")
        void LoadDDS( const char* path );
        /**
          Loads texture from stb_image.c supported formats.

          \param textureData Texture data.
          */
        void LoadSTB( const System::FileContentsData& textureData );

        int width = 0;
        int height = 0;
        unsigned id = 0;
        bool opaque = true;
    };
}
#endif
