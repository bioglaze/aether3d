#include "Texture2D.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.c"
#include "System.hpp"

void ae3d::Texture2D::Load(const System::FileContentsData& fileContents)
{
    int components;
    unsigned char* data = stbi_load(fileContents.path.c_str(), &width, &height, &components, 4);

    if (!data)
    {
        const std::string reason(stbi_failure_reason());
        //Log::Print("%s failed to load. stb_image's reason: %s", fileContents.path.c_str(), reason.c_str());
    }
    
    stbi_image_free(data);
}
