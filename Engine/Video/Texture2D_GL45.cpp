#include "Texture2D.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.c"
#include "System.hpp"
#if __APPLE__
#include <OpenGL/GL.h>
#endif
#if _MSC_VER
#include <GL/glxw.h>
#endif

void ae3d::Texture2D::Load(const System::FileContentsData& fileContents)
{
    int components;
    unsigned char* data = stbi_load(fileContents.path.c_str(), &width, &height, &components, 4);

    if (!data)
    {
        const std::string reason(stbi_failure_reason());
        System::Print("%s failed to load. stb_image's reason: %s", fileContents.path.c_str(), reason.c_str());
        return;
    }
    
    glGenTextures( 1, &id );
    glBindTexture( GL_TEXTURE_2D, id );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

    stbi_image_free(data);
}
