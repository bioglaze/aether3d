#include "TextureCube.hpp"

void ae3d::TextureCube::Load( const FileSystem::FileContentsData& negX, const FileSystem::FileContentsData& posX,
    const FileSystem::FileContentsData& negY, const FileSystem::FileContentsData& posY,
    const FileSystem::FileContentsData& negZ, const FileSystem::FileContentsData& posZ,
    TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace )
{
    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;
    colorSpace = aColorSpace;
}
