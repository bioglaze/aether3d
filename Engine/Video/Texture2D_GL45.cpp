#include "Texture2D.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.c"
#include "System.hpp"
#include <GL/glxw.h>
#include "GfxDevice.hpp"
#include "FileWatcher.hpp"

extern ae3d::FileWatcher fileWatcher;

void TexReload( const std::string& path )
{
    ae3d::System::Print("texture reload (unimplemented): %s", path.c_str());
}

void ae3d::Texture2D::Load(const System::FileContentsData& fileContents)
{
    int components;
    unsigned char* data = stbi_load_from_memory(&fileContents.data[0], static_cast<int>(fileContents.data.size()), &width, &height, &components, 4);

    if (!data)
    {
        const std::string reason(stbi_failure_reason());
        System::Print("%s failed to load. stb_image's reason: %s", fileContents.path.c_str(), reason.c_str());
        return;
    }
    
    id = GfxDevice::CreateTextureId();
    glBindTexture( GL_TEXTURE_2D, id );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

    fileWatcher.AddFile( fileContents.path, TexReload );
    stbi_image_free(data);
}
