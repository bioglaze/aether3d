#ifndef TEXTURE_2D_H
#define TEXTURE_2D_H

namespace ae3d
{
    namespace System
    {
        struct FileContentsData;
    }

    class Texture2D
    {
    public:
        void Load( const System::FileContentsData& textureData );

    private:
        int width;
        int height;
    };
}
#endif
