#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

namespace ae3d
{
    class VertexBuffer
    {
    public:
        void GenerateQuad();
        void Draw() const;

    private:
        unsigned id = 0;
        int elementCount = 0;
    };
}
#endif
