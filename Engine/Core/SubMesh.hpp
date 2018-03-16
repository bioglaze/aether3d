#ifndef SUBMESH_H
#define SUBMESH_H

#include <string>
#include <vector>
#include "VertexBuffer.hpp"
#include "Vec3.hpp"

namespace ae3d
{
    struct Joint
    {
        Matrix44 globalBindposeInverse;
        std::vector< Matrix44 > animTransforms;
        int parentIndex = -1;
        std::string name;
    };

    struct SubMesh
    {
        Vec3 aabbMin;
        Vec3 aabbMax;
        VertexBuffer vertexBuffer;
        std::string name;
        std::vector< VertexBuffer::VertexPTNTC > verticesPTNTC;
        std::vector< VertexBuffer::VertexPTNTC_Skinned > verticesPTNTC_Skinned;
        std::vector< VertexBuffer::VertexPTN > verticesPTN;
        std::vector< VertexBuffer::Face > indices;
        std::vector< Joint > joints;
    };
}

#endif
