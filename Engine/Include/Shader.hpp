#ifndef SHADER_H
#define SHADER_H

#include <map>
#include <string>

namespace ae3d
{
    class Texture2D;

    class Shader
    {
    public:
        void Load( const char* vertexSource, const char* fragmentSource );
        void Use();
        void SetMatrix( const char* name, const float* matrix4x4 );
        void SetTexture( const char* name, const Texture2D* texture, int textureUnit );
        void SetInt( const char* name, int value );
        void SetFloat( const char* name, float value );
        void SetVector3( const char* name, float* vec3 );
        void SetVector4( const char* name, float* vec4 );

    private:
        unsigned id = 0;
        std::map<std::string, int> uniformLocations;
    };    
}
#endif
