#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#include "Vec3.hpp"

namespace ae3d
{
    class VertexBuffer
    {
    public:
        struct Face
        {
            Face() : a(0), b(0), c(0) {}
            
            Face(unsigned short fa, unsigned short fb, unsigned short fc)
            : a(fa)
            , b(fb)
            , c(fc)
            {}
            
            unsigned short a, b, c;
        };

        struct VertexPT
        {
            VertexPT() {}
            VertexPT(const Vec3& pos, float aU, float aV)
            : position(pos)
            , u(aU)
            , v(aV)
            {
            }
            
            Vec3 position;
            float u, v;
        };

        void Generate( const Face* faces, int faceCount, const VertexPT* vertices, int vertexCount );
        void Draw() const;

    private:
        unsigned id = 0;
        int elementCount = 0;
    };
}
#endif
