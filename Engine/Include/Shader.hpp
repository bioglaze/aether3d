#ifndef SHADER_H
#define SHADER_H

namespace ae3d
{
    class Shader
    {
    public:
        void Load(const char* vertexSource, const char* fragmentSource);

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
