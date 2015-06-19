#ifndef TEXTURE_CUBE_H
#define TEXTURE_CUBE_H

#include "TextureBase.hpp"

namespace ae3d
{
    namespace FileSystem
    {
        struct FileContentsData;
    }
    
    /// Cube Map texture.
    class TextureCube : public TextureBase
    {
  public:
        /**
          \param negX Negative X axis texture.
          \param posX Positive X axis texture.
          \param negY Negative X axis texture.
          \param posY Positive X axis texture.
          \param negZ Negative X axis texture.
          \param posZ Positive X axis texture.
          \param wrap Wrapping mode.
          \param filter Filtering mode.
          \param mipmaps Should mipmaps be generated and used.
         */
        void Load( const FileSystem::FileContentsData& negX, const FileSystem::FileContentsData& posX,
                   const FileSystem::FileContentsData& negY, const FileSystem::FileContentsData& posY,
                   const FileSystem::FileContentsData& negZ, const FileSystem::FileContentsData& posZ,
                   TextureWrap wrap, TextureFilter filter, Mipmaps mipmaps );
    };
}

#endif
