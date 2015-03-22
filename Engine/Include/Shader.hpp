#ifndef SHADER_H
#define SHADER_H

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

    private:
        unsigned id = 0;
    };
    
    // TODO: Move into a more appropriate place when found.
    struct BuiltinShaders
    {
        void Load();

        Shader spriteRendererShader;
    };    
}
#endif
