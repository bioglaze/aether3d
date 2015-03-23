#include "VertexBuffer.hpp"
#include <vector>
#include <GL/glxw.h>
#include "Vec3.hpp"

struct VertexPT
{
    VertexPT() {}
    VertexPT(const ae3d::Vec3& pos, float aU, float aV)
        : position(pos)
        , u(aU)
        , v(aV)
    {
    }

    ae3d::Vec3 position;
    float u, v;
};

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

void ae3d::VertexBuffer::GenerateQuad()
{
    std::vector< VertexPT > quadVertices;
    const float s = 100;
    quadVertices.emplace_back(VertexPT(Vec3(-s, -s, 0), 0, 0));
    quadVertices.emplace_back(VertexPT(Vec3(s, -s, 0), 1, 0));
    quadVertices.emplace_back(VertexPT(Vec3(s, s, 0), 1, 1));
    quadVertices.emplace_back(VertexPT(Vec3(-s, s, 0), 0, 1));

    std::vector< Face > indices;
    indices.emplace_back(Face(0, 1, 2));
    indices.emplace_back(Face(2, 3, 0));
    
    elementCount = 2 * 3;

    glGenVertexArrays(1, &id);
    glBindVertexArray(id);

    unsigned vboId;

    glGenBuffers(1, &vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferData(GL_ARRAY_BUFFER, (GLsizei)quadVertices.size() *
        sizeof(VertexPT), &quadVertices[0], GL_STATIC_DRAW);

    unsigned iboId;
    glGenBuffers(1, &iboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei)indices.size() *
        sizeof(unsigned short) * 3, &indices[0], GL_STATIC_DRAW);

    // Vertex
    const int posChannel = 0;
    glEnableVertexAttribArray(posChannel);
    glVertexAttribPointer(posChannel, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPT), nullptr);

    // TexCoord.
    const int uvChannel = 1;
    glEnableVertexAttribArray(uvChannel);
    glVertexAttribPointer(uvChannel, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPT), (const GLvoid*)(3 * 4));

}

void ae3d::VertexBuffer::Draw() const
{
    glBindVertexArray(id);
    glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_SHORT, 0);
}
