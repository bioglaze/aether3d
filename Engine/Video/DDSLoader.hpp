#ifndef DDSLOADER_H
#define DDSLOADER_H

/**
   Holds info about DDS (Direct Draw Surface) image and contains a method to load a .dds file.
 */
namespace DDSLoader
{
    /**
     Loads a .dds file and stores it to currently bound texture.
     Texture must be bound before calling this method.

     \param path Path of .dds file.
     \param cubeMapFace Cube map face index 1-6. For 2D textures use 0.
     \param outWidth Stores the width of the texture in pixels.
     \param outHeight Stores the height of the texture in pixels.
     \param outOpaque Stores info about alpha channel.
     \return True, if loading was successful, false otherwise.
     */
    bool Load( const char* path, int cubeMapFace, int& outWidth, int& outHeight, bool& outOpaque );
}
#endif
