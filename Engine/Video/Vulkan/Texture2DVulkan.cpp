#include "Texture2D.hpp"
#include "System.hpp"

ae3d::Texture2D defaultTexture;

void ae3d::Texture2D::Load( const FileSystem::FileContentsData& fileContents, TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace, float aAnisotropy )
{
    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;
    anisotropy = aAnisotropy;
    colorSpace = aColorSpace;
    handle = 1;
    width = 256;
    height = 256;
}

const ae3d::Texture2D* ae3d::Texture2D::GetDefaultTexture()
{
    defaultTexture.handle = 1;
    defaultTexture.width = 256;
    defaultTexture.height = 256;

    return &defaultTexture;
}
