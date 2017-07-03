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
        ae3d::Matrix44 globalBindposeInverse;
        std::vector< ae3d::Matrix44 > animTransforms;
        int parentIndex = -1;
        std::string name;
    };

    struct SubMesh
    {
        ae3d::Vec3 aabbMin;
        ae3d::Vec3 aabbMax;
        ae3d::VertexBuffer vertexBuffer;
        std::string name;
        std::vector< VertexBuffer::VertexPTNTC > verticesPTNTC;
        std::vector< VertexBuffer::VertexPTNTC_Skinned > verticesPTNTC_Skinned;
        std::vector< VertexBuffer::VertexPTN > verticesPTN;
        std::vector< VertexBuffer::Face > indices;
        std::vector< Joint > joints;
    };
}

#endif
