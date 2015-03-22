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
        unsigned GetID() const { return id; }
        
    private:
        int width = 0;
        int height = 0;
        unsigned id = 0;
    };
}
#endif
