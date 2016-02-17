#include "Texture2D.hpp"

void ae3d::Texture2D::Load( const FileSystem::FileContentsData& fileContents, TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace, float aAnisotropy )
{
    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;
    anisotropy = aAnisotropy;
    colorSpace = aColorSpace;
}

const ae3d::Texture2D* ae3d::Texture2D::GetDefaultTexture()
{
    return nullptr;
}
