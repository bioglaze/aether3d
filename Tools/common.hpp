// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#ifndef COMMON_H
#define COMMON_H

#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include "Matrix.hpp"
#include "Vec3.hpp"

// Cache optimization code adapted from http://gameangst.com/wp-content/uploads/2009/03/forsythtriangleorderoptimizer.cpp

static unsigned short Min( unsigned short a, unsigned short b )
{
    return a < b ? a : b;
}

struct TexCoord
{
    TexCoord() : u( 0 ), v( 0 ) {}

    TexCoord( float aU, float aV ) : u( aU ), v( aV ) {}

    TexCoord operator-( const TexCoord& t ) const
    {
        return TexCoord( u - t.u, v - t.v );
    }

    TexCoord operator+( const TexCoord& t ) const
    {
        return TexCoord( u + t.u, v + t.v );
    }

    float u, v;
};

/**
 Used to store intermediate vertex data that comes from
 the exported model.  Later it's converted to vertex array
 friendly format by removing duplicates and interleaving data.
 */
struct Face
{
    Face()
    {
        vInd[ 0 ] = vInd[ 1 ] = vInd[ 2 ] = 0;
        uvInd[ 0 ] = uvInd[ 1 ] = uvInd[ 2 ] = 0;
        vnInd[ 0 ] = vnInd[ 1 ] = vnInd[ 2 ] = 0;
        colInd[ 0 ] = colInd[ 1 ] = colInd[ 2 ] = 0;
        tInd[ 0 ] = tInd[ 1 ] = tInd[ 2 ] = 0;
    }

    unsigned int vInd[ 3 ];
    unsigned int uvInd[ 3 ];
    unsigned int vnInd[ 3 ];
    unsigned int colInd[ 3 ];
    unsigned int tInd[ 3 ];
};

// Defines a face used in a vertex array.
// a, b and c are indices to Mesh::interleavedVertices.
struct VertexInd
{
    unsigned short a, b, c;
};

enum class VertexFormat { PTNTC_Skinned, PTNTC, PTN };

struct VertexPTNTC_Skinned
{
    ae3d::Vec3 position;
    TexCoord texCoord;
    ae3d::Vec3 normal;
    ae3d::Vec4 tangent;
    ae3d::Vec4 color;
    ae3d::Vec4 weights;
    int bones[ 4 ];
};

struct VertexPTNTC
{
    ae3d::Vec3 position;
    TexCoord texCoord;
    ae3d::Vec3 normal;
    ae3d::Vec4 tangent;
    ae3d::Vec4 color;
};

struct VertexPTN
{
    ae3d::Vec3 position;
    TexCoord texCoord;
    ae3d::Vec3 normal;
};

struct VertexData
{
    float    score = 0;
    unsigned activeFaceListStart = 0;
    unsigned activeFaceListSize = 0;
    unsigned cachePos0 = 0;
    unsigned cachePos1 = 0;
};

struct VertexPTNTCWithData
{
    ae3d::Vec3 position;
    TexCoord texCoord;
    ae3d::Vec3 normal;
    ae3d::Vec4 tangent;
    ae3d::Vec4 color;
    VertexData data;
};

struct Joint
{
    ae3d::Matrix44 globalBindposeInverse;
    int parentIndex = -1;
    std::string name;
    std::vector< ae3d::Matrix44 > animTransforms;
};

struct BoneIndices
{
    int a, b, c, d;
};

struct Mesh
{
    void Interleave();
    void SolveAABB();
    void SolveFaceNormals();
    void SolveFaceTangents();
    void SolveVertexNormals();
    void SolveVertexTangents();
    void CopyInterleavedVerticesToPTN();
    void CopyInterleavedVerticesToPTNTC();
    
    void OptimizeFaces(); // Implements https://tomforsyth1000.github.io/papers/fast_vert_cache_opt.html
    bool ComputeVertexScores();

    bool AlmostEquals( const ae3d::Vec3& v1, const ae3d::Vec3& v2 ) const;
    bool AlmostEquals( const ae3d::Vec4& v1, const ae3d::Vec4& v2 ) const;
    bool AlmostEquals( const TexCoord& t1, const TexCoord& t2 ) const;

    std::string name = { "unnamed" };
    std::vector< ae3d::Vec3 > vertex;
    std::vector< ae3d::Vec4 > tangents;
    std::vector< ae3d::Vec4 > nonInterleavedTangents;
    std::vector< ae3d::Vec3 > vnormal;
    std::vector< ae3d::Vec4 > colors;
    std::vector< ae3d::Vec3 > fnormal;
    std::vector< TexCoord > tcoord;
    std::vector< Face >     face;
    std::vector< VertexPTNTCWithData > verticesWithCachedata;
    
    // Separate element arrays combined to one.
    // These are written to the .ae3d file.
    std::vector< VertexPTNTC_Skinned > interleavedVertices;
    std::vector< VertexPTNTC > interleavedVerticesPTNTC;
    std::vector< VertexPTN > interleavedVerticesPTN;
    std::vector< VertexInd > indices;

    // Used to calculate tangent-space handedness.
    std::vector< ae3d::Vec3 > bitangents;  // For faces.

    ae3d::Vec3 aabbMax;
    ae3d::Vec3 aabbMin;

    std::vector< Joint > joints;
    std::vector< ae3d::Vec4 > weights;
    std::vector< BoneIndices > bones;
};

std::vector< Mesh > gMeshes;

const unsigned MaxVertexCacheSize = 64;
const unsigned MaxPrecomputedVertexValenceScores = 64;
const unsigned short lruCacheSize = 64;

float gVertexCacheScores[ MaxVertexCacheSize + 1 ][ MaxVertexCacheSize ];
float gVertexValenceScores[ MaxPrecomputedVertexValenceScores ];

std::vector< VertexInd > newIndexList;

void Mesh::CopyInterleavedVerticesToPTN()
{
    interleavedVerticesPTN.resize( interleavedVertices.size() );
    
    for (size_t i = 0; i < interleavedVertices.size(); ++i)
    {
        interleavedVerticesPTN[ i ].position = interleavedVertices[ i ].position;
        interleavedVerticesPTN[ i ].texCoord = interleavedVertices[ i ].texCoord;
        interleavedVerticesPTN[ i ].normal = interleavedVertices[ i ].normal;
    }
}

void Mesh::CopyInterleavedVerticesToPTNTC()
{
    interleavedVerticesPTNTC.resize( interleavedVertices.size() );
    
    for (size_t i = 0; i < interleavedVerticesPTNTC.size(); ++i)
    {
        interleavedVerticesPTNTC[ i ].position = interleavedVertices[ i ].position;
        interleavedVerticesPTNTC[ i ].texCoord = interleavedVertices[ i ].texCoord;
        interleavedVerticesPTNTC[ i ].normal = interleavedVertices[ i ].normal;
        interleavedVerticesPTNTC[ i ].tangent = interleavedVertices[ i ].tangent;
        interleavedVerticesPTNTC[ i ].color = interleavedVertices[ i ].color;
    }
}

float ComputeVertexCacheScore( int cachePosition, int vertexCacheSize )
{
    const float findVertexScore_CacheDecayPower = 1.5f;
    const float findVertexScore_LastTriScore = 0.75f;

    float score = 0;

    if (cachePosition < 0)
    {
        // Vertex is not in FIFO -> no score
    }
    else
    {
        if (cachePosition < 3)
        {
            // This vertex was used in the last triangle,
            // so it has a fixed score, whichever of the three
            // it's in. Otherwise, you can get very different
            // answers depending on whether you add
            // the triangle 1,2,3 or 3,1,2 - which is silly.
            score = findVertexScore_LastTriScore;
        }
        else
        {
            assert( cachePosition < vertexCacheSize && "Cache position exceeds the cache size" );
            const float scaler = 1.0f / (vertexCacheSize - 3);
            score = 1.0f - (cachePosition - 3) * scaler;
            score = std::pow( score, findVertexScore_CacheDecayPower );
        }
    }

    return score;
}

float ComputeVertexValenceScore( unsigned numActiveFaces )
{
    const float FindVertexScore_ValenceBoostScale = 2.0f;
    const float FindVertexScore_ValenceBoostPower = 0.5f;

    float score = 0;

    // Bonus points for having a low number of tris still to
    // use the vert, so we get rid of lone verts quickly.
    float valenceBoost = std::pow( static_cast< float >(numActiveFaces),
        -FindVertexScore_ValenceBoostPower );
    score += FindVertexScore_ValenceBoostScale * valenceBoost;

    return score;
}

float FindVertexScore( unsigned numActiveFaces, unsigned cachePosition, unsigned vertexCacheSize )
{
    if (numActiveFaces == 0)
    {
        // No tri needs this vertex!
        return -1.0f;
    }

    float score = 0.f;
    if (cachePosition < vertexCacheSize)
    {
        score += gVertexCacheScores[ vertexCacheSize ][ cachePosition ];
    }

    if (numActiveFaces < MaxPrecomputedVertexValenceScores)
    {
        score += gVertexValenceScores[ numActiveFaces ];
    }
    else
    {
        score += ComputeVertexValenceScore( numActiveFaces );
    }

    return score;
}

bool Mesh::ComputeVertexScores()
{
    for (unsigned cacheSize = 0; cacheSize <= MaxVertexCacheSize; ++cacheSize)
    {
        for (unsigned cachePos = 0; cachePos < cacheSize; ++cachePos)
        {
            gVertexCacheScores[ cacheSize ][ cachePos ] = ComputeVertexCacheScore( cachePos, cacheSize );
        }
    }
    
    for (unsigned valence = 0; valence < MaxPrecomputedVertexValenceScores; ++valence)
    {
        gVertexValenceScores[ valence ] = ComputeVertexValenceScore( valence );
    }
    
    return true;
}

void Mesh::OptimizeFaces()
{
    verticesWithCachedata.resize( interleavedVertices.size() );

    for (std::size_t i = 0; i < interleavedVertices.size(); ++i)
    {
        verticesWithCachedata[ i ].color = interleavedVertices[ i ].color;
        verticesWithCachedata[ i ].texCoord = interleavedVertices[ i ].texCoord;
        verticesWithCachedata[ i ].position = interleavedVertices[ i ].position;
        verticesWithCachedata[ i ].tangent = interleavedVertices[ i ].tangent;
        verticesWithCachedata[ i ].normal = interleavedVertices[ i ].normal;
    }

    // Face count per vertex
    for (std::size_t i = 0; i < indices.size(); ++i)
    {
        ++verticesWithCachedata[ indices[ i ].a ].data.activeFaceListSize;
        ++verticesWithCachedata[ indices[ i ].b ].data.activeFaceListSize;
        ++verticesWithCachedata[ indices[ i ].c ].data.activeFaceListSize;
    }
    
    std::vector< unsigned > activeFaceList;

    const unsigned short kEvictedCacheIndex = std::numeric_limits<unsigned short>::max();

    {
        // allocate face list per vertex
        unsigned curActiveFaceListPos = 0;

        for (std::size_t i = 0; i < verticesWithCachedata.size(); ++i)
        {
            VertexPTNTCWithData& vertexData = verticesWithCachedata[ i ];
            vertexData.data.cachePos0 = kEvictedCacheIndex;
            vertexData.data.cachePos1 = kEvictedCacheIndex;
            vertexData.data.activeFaceListStart = curActiveFaceListPos;
            curActiveFaceListPos += vertexData.data.activeFaceListSize;
            vertexData.data.score = FindVertexScore( vertexData.data.activeFaceListSize, vertexData.data.cachePos0, lruCacheSize );
            vertexData.data.activeFaceListSize = 0;
        }
        activeFaceList.resize( curActiveFaceListPos );
    }

    // fill out face list per vertex
    for (unsigned i = 0; i < static_cast< unsigned >( indices.size() ); ++i)
    {
        unsigned short index = indices[ i ].a;
        VertexPTNTCWithData& vertexDataA = verticesWithCachedata[ index ];
        activeFaceList[ vertexDataA.data.activeFaceListStart + vertexDataA.data.activeFaceListSize ] = i;
        ++vertexDataA.data.activeFaceListSize;

        index = indices[ i ].b;
        VertexPTNTCWithData& vertexDataB = verticesWithCachedata[ index ];
        activeFaceList[ vertexDataB.data.activeFaceListStart + vertexDataB.data.activeFaceListSize ] = i;
        ++vertexDataB.data.activeFaceListSize;

        index = indices[ i ].c;
        VertexPTNTCWithData& vertexDataC = verticesWithCachedata[ index ];
        activeFaceList[ vertexDataC.data.activeFaceListStart + vertexDataC.data.activeFaceListSize ] = i;
        ++vertexDataC.data.activeFaceListSize;
    }

    std::vector<std::uint8_t> processedFaceList;
    processedFaceList.resize( indices.size() );

    unsigned short vertexCacheBuffer[ (MaxVertexCacheSize + 3) * 2 ];
    unsigned short* cache0 = vertexCacheBuffer;
    unsigned short* cache1 = vertexCacheBuffer + (MaxVertexCacheSize + 3);
    unsigned short entriesInCache0 = 0;

    unsigned bestFace = 0;
    float bestScore = -1.f;

    const float maxValenceScore = FindVertexScore( 1, kEvictedCacheIndex, lruCacheSize ) * 3.0f;

    newIndexList.resize( indices.size() );

    for (unsigned i = 0; i < indices.size(); ++i)
    {
        if (bestScore < 0.f)
        {
            // no verts in the cache are used by any unprocessed faces so
            // search all unprocessed faces for a new starting point
            for (unsigned j = 0; j < indices.size(); ++j)
            {
                if (processedFaceList[ j ] == 0)
                {
                    unsigned fface = j;
                    float faceScore = 0.f;
                   
                    unsigned short indexA = indices[ fface ].a;
                    VertexPTNTCWithData& vertexDataA = verticesWithCachedata[ indexA ];
                    assert( vertexDataA.data.activeFaceListSize > 0 );
                    assert( vertexDataA.data.cachePos0 >= lruCacheSize );
                    faceScore += vertexDataA.data.score;

                    unsigned short indexB = indices[ fface ].b;
                    VertexPTNTCWithData& vertexDataB = verticesWithCachedata[ indexB ];
                    assert( vertexDataB.data.activeFaceListSize > 0 );
                    assert( vertexDataB.data.cachePos0 >= lruCacheSize );
                    faceScore += vertexDataB.data.score;

                    unsigned short indexC = indices[ fface ].c;
                    VertexPTNTCWithData& vertexDataC = verticesWithCachedata[ indexC ];
                    assert( vertexDataC.data.activeFaceListSize > 0 );
                    assert( vertexDataC.data.cachePos0 >= lruCacheSize );
                    faceScore += vertexDataC.data.score;

                    if (faceScore > bestScore)
                    {
                        bestScore = faceScore;
                        bestFace = fface;

                        //assert( bestScore <= maxValenceScore );
                        if (bestScore >= maxValenceScore)
                        {
                            break;
                        }
                    }
                }
            }
            assert( bestScore >= 0.f );
        }

        processedFaceList[ bestFace ] = 1;
        unsigned short entriesInCache1 = 0;

        // add bestFace to LRU cache and to newIndexList
        {
            unsigned short indexA = indices[ bestFace ].a;
            newIndexList[ i ].a = indexA;

            VertexPTNTCWithData& vertexData = verticesWithCachedata[ indexA ];
            bool skip = false;

            if (vertexData.data.cachePos1 >= entriesInCache1)
            {
                vertexData.data.cachePos1 = entriesInCache1;
                cache1[ entriesInCache1++ ] = indexA;

                if (vertexData.data.activeFaceListSize == 1)
                {
                    --vertexData.data.activeFaceListSize;
                    skip = true;
                }
            }

            if (!skip)
            {
                assert( vertexData.data.activeFaceListSize > 0 );
                unsigned* begin = &activeFaceList[ vertexData.data.activeFaceListStart ];
                unsigned* end = &activeFaceList[ vertexData.data.activeFaceListStart + vertexData.data.activeFaceListSize ];
                unsigned* it = std::find( begin, end, bestFace );
                assert( it != end );
                std::swap( *it, *(end - 1) );
                --vertexData.data.activeFaceListSize;
                vertexData.data.score = FindVertexScore( vertexData.data.activeFaceListSize, vertexData.data.cachePos1, lruCacheSize );
            }
        }

        {
            unsigned short indexB = indices[ bestFace ].b;
            newIndexList[ i ].b = indexB;

            VertexPTNTCWithData& vertexData = verticesWithCachedata[ indexB ];
            bool skip = false;

            if (vertexData.data.cachePos1 >= entriesInCache1)
            {
                vertexData.data.cachePos1 = entriesInCache1;
                cache1[ entriesInCache1++ ] = indexB;

                if (vertexData.data.activeFaceListSize == 1)
                {
                    --vertexData.data.activeFaceListSize;
                    skip = true;
                }
            }

            if (!skip)
            {
                assert( vertexData.data.activeFaceListSize > 0 );
                unsigned* begin = &activeFaceList[ vertexData.data.activeFaceListStart ];
                unsigned* end = &activeFaceList[ vertexData.data.activeFaceListStart + vertexData.data.activeFaceListSize ];
                auto it = std::find( begin, end, bestFace );
                assert( it != end );
                std::swap( *it, *(end - 1) );
                --vertexData.data.activeFaceListSize;
                vertexData.data.score = FindVertexScore( vertexData.data.activeFaceListSize, vertexData.data.cachePos1, lruCacheSize );
            }
        }

        {
            unsigned short indexC = indices[ bestFace ].c;
            newIndexList[ i ].c = indexC;

            VertexPTNTCWithData& vertexData = verticesWithCachedata[ indexC ];
            bool skip = false;

            if (vertexData.data.cachePos1 >= entriesInCache1)
            {
                vertexData.data.cachePos1 = entriesInCache1;
                cache1[ entriesInCache1++ ] = indexC;

                if (vertexData.data.activeFaceListSize == 1)
                {
                    --vertexData.data.activeFaceListSize;
                    skip = true;
                }
            }

            if (!skip)
            {
                assert( vertexData.data.activeFaceListSize > 0 );
                unsigned* begin = &activeFaceList[ vertexData.data.activeFaceListStart ];
                unsigned* end = &activeFaceList[ vertexData.data.activeFaceListStart + vertexData.data.activeFaceListSize ];
                unsigned* it = std::find( begin, end, bestFace );
                assert( it != end );
                std::swap( *it, *(end - 1) );
                --vertexData.data.activeFaceListSize;
                vertexData.data.score = FindVertexScore( vertexData.data.activeFaceListSize, vertexData.data.cachePos1, lruCacheSize );
            }
        }
        // move the rest of the old verts in the cache down and compute their new scores
        for (unsigned c0 = 0; c0 < entriesInCache0; ++c0)
        {
            unsigned short index = cache0[ c0 ];
            VertexPTNTCWithData& vertexData = verticesWithCachedata[ index ];

            if (vertexData.data.cachePos1 >= entriesInCache1)
            {
                vertexData.data.cachePos1 = entriesInCache1;
                cache1[ entriesInCache1++ ] = index;
                vertexData.data.score = FindVertexScore( vertexData.data.activeFaceListSize, vertexData.data.cachePos1, lruCacheSize );
            }
        }

        // find the best scoring triangle in the current cache (including up to 3 that were just evicted)
        bestScore = -1.f;
        for (unsigned c1 = 0; c1 < entriesInCache1; ++c1)
        {
            unsigned short index = cache1[ c1 ];
            VertexPTNTCWithData& vertexData = verticesWithCachedata[ index ];
            vertexData.data.cachePos0 = vertexData.data.cachePos1;
            vertexData.data.cachePos1 = kEvictedCacheIndex;
            for (unsigned j = 0; j < vertexData.data.activeFaceListSize; ++j)
            {
                unsigned fface = activeFaceList[ vertexData.data.activeFaceListStart + j ];
                float faceScore = 0.f;
                    
                unsigned short faceIndexA = indices[ fface ].a;
                VertexPTNTCWithData& faceVertexDataA = verticesWithCachedata[ faceIndexA ];
                faceScore += faceVertexDataA.data.score;

                unsigned short faceIndexB = indices[ fface ].b;
                VertexPTNTCWithData& faceVertexDataB = verticesWithCachedata[ faceIndexB ];
                faceScore += faceVertexDataB.data.score;

                unsigned short faceIndexC = indices[ fface ].c;
                VertexPTNTCWithData& faceVertexDataC = verticesWithCachedata[ faceIndexC ];
                faceScore += faceVertexDataC.data.score;

                if (faceScore > bestScore)
                {
                    bestScore = faceScore;
                    bestFace = fface;
                }
            }
        }

        std::swap( cache0, cache1 );
        entriesInCache0 = Min( entriesInCache1, lruCacheSize );
    }

    indices = newIndexList;
}

void Mesh::SolveAABB()
{
    const float maxValue = 99999999.0f;

    aabbMin = ae3d::Vec3(  maxValue,  maxValue,  maxValue );
    aabbMax = ae3d::Vec3( -maxValue, -maxValue, -maxValue );

    for (std::size_t v = 0; v < vertex.size(); ++v)
    {
        const ae3d::Vec3& pos = vertex[ v ];

        if (pos.x > maxValue || pos.y > maxValue || pos.z > maxValue)
        {
            std::cerr << "AABB's max value is too low in file " << __FILE__ << " near line " << __LINE__ << std::endl;
            std::cerr << "Vertex " << v << ": " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
        }
        
        if (pos.x < -maxValue || pos.y < -maxValue || pos.z < -maxValue)
        {
            std::cerr << "AABB's min value is too high in file " << __FILE__ << " near line " << __LINE__ << std::endl;
            std::cerr << "Vertex " << v << ": " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
        }

        aabbMin = ae3d::Vec3::Min2( aabbMin, pos );
        aabbMax = ae3d::Vec3::Max2( aabbMax, pos );
    }
}

void Mesh::SolveFaceNormals()
{
    fnormal.resize( indices.size() );

    for (std::size_t faceInd = 0; faceInd < indices.size(); ++faceInd)
    {
        const unsigned short& faceA = indices[ faceInd ].a;
        const unsigned short& faceB = indices[ faceInd ].b;
        const unsigned short& faceC = indices[ faceInd ].c;

        const ae3d::Vec3 va = interleavedVertices[ faceA ].position;
        ae3d::Vec3 vb = interleavedVertices[ faceB ].position;
        ae3d::Vec3 vc = interleavedVertices[ faceC ].position;

        vb -= va;
        vc -= va;

        vb = ae3d::Vec3::Cross( vb, vc ).Normalized();

        fnormal[ faceInd ] = vb;
    }
}

/**
 Generates tangents for faces.

 Tangent vector (u-axis) will point in the direction of increasing U-coordinate.
 Bitangent vector (v-axis) will point in the direction of increasing V-coordinate.
 */
void Mesh::SolveFaceTangents()
{
    assert( !indices.empty() );

    tangents.resize  ( indices.size() );
    bitangents.resize( indices.size() );

    bool degenerateFound = false;

    // Algorithm source:
    // http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/#header-3
    for (std::size_t f = 0; f < indices.size(); ++f)
    {
        const ae3d::Vec3& p1 = interleavedVertices[ indices[ f ].a ].position;
        const ae3d::Vec3& p2 = interleavedVertices[ indices[ f ].b ].position;
        const ae3d::Vec3& p3 = interleavedVertices[ indices[ f ].c ].position;

        const TexCoord& uv1 = interleavedVertices[ indices[ f ].a ].texCoord;
        const TexCoord& uv2 = interleavedVertices[ indices[ f ].b ].texCoord;
        const TexCoord& uv3 = interleavedVertices[ indices[ f ].c ].texCoord;

        const TexCoord deltaUV1 = uv2 - uv1;
        const TexCoord deltaUV2 = uv3 - uv1;

        const float area = deltaUV1.u * deltaUV2.v - deltaUV1.v * deltaUV2.u;

        ae3d::Vec3 tangent;
        ae3d::Vec3 bitangent;

        if (std::fabs( area ) > 0.00001f)
        {
            const ae3d::Vec3 deltaP1 = p2 - p1;
            const ae3d::Vec3 deltaP2 = p3 - p1;

            tangent   = (deltaP1 * deltaUV2.v - deltaP2 * deltaUV1.v) / area;
            bitangent = (deltaP2 * deltaUV1.u - deltaP1 * deltaUV2.u) / area;
        }
        else
        {
            degenerateFound = true;
        }

        const ae3d::Vec4 tangent4( tangent.x, tangent.y, tangent.z, 0.0f );
        tangents[ f ]   = tangent4;
        bitangents[ f ] = bitangent;
    }

    if (degenerateFound)
    {
        std::cout << std::endl << "Warning: Degenerate UV map in mesh " << name << ".  Author needs to separate texture points." << std::endl;
    }
}

void Mesh::SolveVertexNormals()
{
    vnormal.resize( vertex.size() );
    
    // For every vertex, check how many faces touches
    // it and calculate the average of those face normals.
    for (std::size_t vertInd = 0; vertInd < vertex.size(); ++vertInd)
    {
        ae3d::Vec3 normal;

        for (std::size_t faceInd = 0; faceInd < face.size(); ++faceInd)
        {
            face[ faceInd ].vnInd[ 0 ] = face[ faceInd ].vInd[ 0 ];
            face[ faceInd ].vnInd[ 1 ] = face[ faceInd ].vInd[ 1 ];
            face[ faceInd ].vnInd[ 2 ] = face[ faceInd ].vInd[ 2 ];

            const unsigned short& faceA = face[ faceInd ].vInd[ 0 ];
            const unsigned short& faceB = face[ faceInd ].vInd[ 1 ];
            const unsigned short& faceC = face[ faceInd ].vInd[ 2 ];

            const ae3d::Vec3 va = vertex[ faceA ];
            ae3d::Vec3 vb = vertex[ faceB ] - va;
            const ae3d::Vec3 vc = vertex[ faceC ] - va;

            vb = ae3d::Vec3::Cross( vb, vc ).Normalized();

            if (face[ faceInd ].vInd[ 0 ] == vertInd ||
                face[ faceInd ].vInd[ 1 ] == vertInd ||
                face[ faceInd ].vInd[ 2 ] == vertInd)
            {
                normal = normal + vb;
            }
        }
        
        vnormal[ vertInd ] = normal.Normalized();
    }
}

void Mesh::SolveVertexTangents()
{
    assert( !tangents.empty() );

    for (std::size_t v = 0; v < interleavedVertices.size(); ++v)
    {
        interleavedVertices[ v ].tangent = ae3d::Vec4( 0, 0, 0, 0 );
    }

    std::vector< ae3d::Vec3 > vbitangents( interleavedVertices.size() );

    for (std::size_t vertInd = 0; vertInd < interleavedVertices.size(); ++vertInd)
    {
        ae3d::Vec4 tangent( 0.0f, 0.0f, 0.0f, 0.0f );
        ae3d::Vec3 bitangent;

        for (std::size_t faceInd = 0; faceInd < indices.size(); ++faceInd)
        {
            if (indices[ faceInd ].a == vertInd ||
                indices[ faceInd ].b == vertInd ||
                indices[ faceInd ].c == vertInd)
            {
                tangent   += tangents  [ faceInd ];
                bitangent += bitangents[ faceInd ];
            }
        }

        interleavedVertices[ vertInd ].tangent = tangent;
        vbitangents[ vertInd ] = bitangent;
    }

    for (std::size_t v = 0; v < interleavedVertices.size(); ++v)
    {
        ae3d::Vec4& tangent = interleavedVertices[ v ].tangent;
        const ae3d::Vec3& normal = interleavedVertices[ v ].normal;
        const ae3d::Vec4 normal4( normal.x, normal.y, normal.z, 0 );

        // Gram-Schmidt orthonormalization.
        tangent -= normal4 * normal4.Dot( tangent );
        tangent.Normalize();

        // Handedness. TBN must form a right-handed coordinate system,
        // i.e. cross(n,t) must have the same orientation as b.
        const ae3d::Vec3 cp = ae3d::Vec3::Cross( normal, ae3d::Vec3( tangent.x, tangent.y, tangent.z ) );
        tangent.w = ae3d::Vec3::Dot( cp, vbitangents[v] ) >= 0 ? 1.0f : -1.0f;
    }
}

// Creates an interleaved vertex array.
void Mesh::Interleave()
{
    VertexInd newFace;

    for (std::size_t f = 0; f < face.size(); ++f)
    {
        // vertind 0
        ae3d::Vec3 tvertex = vertex [ face[ f ].vInd [ 0 ] ];
        ae3d::Vec3 tnormal = vnormal[ face[ f ].vnInd[ 0 ] ];
        TexCoord ttcoord = tcoord.empty() ? TexCoord() : tcoord[ face[ f ].uvInd[ 0 ] ];
        ae3d::Vec4 tcolor = colors.empty() ? ae3d::Vec4( 0, 0, 0, 1 ) : colors[ face[ f ].colInd[ 0 ] ];
        ae3d::Vec4 ttangent = nonInterleavedTangents.empty() ? ae3d::Vec4( 0, 0, 0, 1 ) : nonInterleavedTangents[ face[ f ].tInd[ 0 ] ];

        // Searches face f's vertex a from the vertex list.
        // If it's not found, it's added.
        bool found = false;
        
        for (std::size_t i = 0; i < indices.size(); ++i)
        {
            if (AlmostEquals( interleavedVertices[ indices[ i ].a ].position, tvertex ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].a ].normal,   tnormal ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].a ].texCoord, ttcoord ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].a ].tangent, ttangent ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].a ].color, tcolor ))
            {
                found = true;
                newFace.a = indices[ i ].a;
                break;
            }
        }

        if (!found)
        {
            VertexPTNTC_Skinned newVertex;
            newVertex.position = tvertex;
            newVertex.normal   = tnormal;
            newVertex.texCoord = ttcoord;
            newVertex.color    = tcolor;
            newVertex.tangent  = ttangent;
            
            if (!weights.empty())
            {
                newVertex.weights  = weights[ face[ f ].vInd[ 0 ] ];
            }
            if (!bones.empty())
            {
                newVertex.bones[ 0 ] = bones[ face[ f ].vInd[ 0 ] ].a;
                newVertex.bones[ 1 ] = bones[ face[ f ].vInd[ 0 ] ].b;
                newVertex.bones[ 2 ] = bones[ face[ f ].vInd[ 0 ] ].c;
                newVertex.bones[ 3 ] = bones[ face[ f ].vInd[ 0 ] ].d;
            }
            
            interleavedVertices.push_back( newVertex );
            
            newFace.a = (unsigned short)(interleavedVertices.size() - 1);
        }

        // vertind 1
        tvertex = vertex [ face[ f ].vInd [ 1 ] ];
        tnormal = vnormal[ face[ f ].vnInd[ 1 ] ];
        ttcoord = tcoord.empty() ? TexCoord() : tcoord [ face[ f ].uvInd[ 1 ] ];
        tcolor = colors.empty() ? ae3d::Vec4( 0, 0, 0, 1 ) : colors[ face[ f ].colInd[ 1 ] ];
        ttangent = nonInterleavedTangents.empty() ? ae3d::Vec4( 0, 0, 0, 1 ) : nonInterleavedTangents[ face[ f ].tInd[ 1 ] ];
        
        found = false;

        for (std::size_t i = 0; i < indices.size(); ++i)
        {
            if (AlmostEquals( interleavedVertices[ indices[ i ].b ].position, tvertex ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].b ].normal,   tnormal ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].b ].texCoord, ttcoord ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].b ].tangent, ttangent ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].b ].color, tcolor ))
                
            {
                found = true;

                newFace.b = indices[ i ].b;
                break;
            }
        }

        if (!found)
        {
            VertexPTNTC_Skinned newVertex;
            newVertex.position = tvertex;
            newVertex.normal   = tnormal;
            newVertex.texCoord = ttcoord;
            newVertex.color    = tcolor;
            newVertex.tangent  = ttangent;
            
            if (!weights.empty())
            {
                newVertex.weights  = weights[ face[ f ].vInd[ 1 ] ];
            }
            if (!bones.empty())
            {
                newVertex.bones[ 0 ] = bones[ face[ f ].vInd[ 1 ] ].a;
                newVertex.bones[ 1 ] = bones[ face[ f ].vInd[ 1 ] ].b;
                newVertex.bones[ 2 ] = bones[ face[ f ].vInd[ 1 ] ].c;
                newVertex.bones[ 3 ] = bones[ face[ f ].vInd[ 1 ] ].d;
            }
            
            interleavedVertices.push_back( newVertex );

            newFace.b = (unsigned short)(interleavedVertices.size() - 1);
        }

        // vertind 2
        tvertex = vertex [ face[ f ].vInd [ 2 ] ];
        tnormal = vnormal[ face[ f ].vnInd[ 2 ] ];
        ttcoord = tcoord.empty() ? TexCoord() :  tcoord [ face[ f ].uvInd[ 2 ] ];
        tcolor = colors.empty() ? ae3d::Vec4( 0, 0, 0, 1 ) : colors[ face[ f ].colInd[ 2 ] ];
        ttangent = nonInterleavedTangents.empty() ? ae3d::Vec4( 0, 0, 0, 1 ) : nonInterleavedTangents[ face[ f ].tInd[ 2 ] ];

        found = false;

        for (std::size_t i = 0; i < indices.size(); ++i)
        {
            if (AlmostEquals( interleavedVertices[ indices[ i ].c ].position, tvertex ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].c ].normal,   tnormal ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].c ].texCoord, ttcoord ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].c ].tangent, ttangent ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].c ].color, tcolor ))
            {
                found = true;

                newFace.c = indices[ i ].c;
                break;
            }
        }

        if (!found)
        {
            VertexPTNTC_Skinned newVertex;
            newVertex.position = tvertex;
            newVertex.normal   = tnormal;
            newVertex.texCoord = ttcoord;
            newVertex.color    = tcolor;
            newVertex.tangent  = ttangent;

            if (!weights.empty())
            {
                newVertex.weights  = weights[ face[ f ].vInd[ 2 ] ];
            }
            if (!bones.empty())
            {
                newVertex.bones[ 0 ] = bones[ face[ f ].vInd[ 2 ] ].a;
                newVertex.bones[ 1 ] = bones[ face[ f ].vInd[ 2 ] ].b;
                newVertex.bones[ 2 ] = bones[ face[ f ].vInd[ 2 ] ].c;
                newVertex.bones[ 3 ] = bones[ face[ f ].vInd[ 2 ] ].d;
            }
            
            interleavedVertices.push_back( newVertex );

            newFace.c = (unsigned short)(interleavedVertices.size() - 1);
        }

        indices.push_back( newFace );
    }

    if (interleavedVertices.size() > 65536)
    {
        std::cerr << "Mesh " << name << " has more than 65536 vertices!" << std::endl;
        exit( 1 );
    }
}

bool Mesh::AlmostEquals( const ae3d::Vec3& v1, const ae3d::Vec3& v2 ) const
{
    if (std::fabs( v1.x - v2.x ) > 0.0001f) { return false; }
    if (std::fabs( v1.y - v2.y ) > 0.0001f) { return false; }
    if (std::fabs( v1.z - v2.z ) > 0.0001f) { return false; }
    return true;
}

bool Mesh::AlmostEquals( const ae3d::Vec4& v1, const ae3d::Vec4& v2 ) const
{
    if (std::fabs( v1.x - v2.x ) > 0.0001f) { return false; }
    if (std::fabs( v1.y - v2.y ) > 0.0001f) { return false; }
    if (std::fabs( v1.z - v2.z ) > 0.0001f) { return false; }
    if (std::fabs( v1.w - v2.w ) > 0.0001f) { return false; }
    return true;
}

bool Mesh::AlmostEquals( const TexCoord& t1, const TexCoord& t2 ) const
{
    if (std::fabs( t1.u - t2.u ) > 0.0001f) { return false; }
    if (std::fabs( t1.v - t2.v ) > 0.0001f) { return false; }
    return true;
}

/**
 bytes  data
 (2)    magic number, e.g. "a7"
 (4*6)  Object's AABB min, AABB max.
 (2)    # of meshes
 (4*6)      Mesh's AABB min, AABB max.
 (2)        mesh name length in bytes.
 (*)        mesh name (1 character = 1 byte)
 (2)        # of vertices.
 (*)        Vertex data array of type Vertex.
 (2)        # of faces
 (*)        faces
 (2)        # of joints if magic number is >= a8
 (*)        joints
 (1)    terminator byte: 100
 */

/// Writes a .ae3d model to a file.
/// \param aOutFile File name to save the model into.
void WriteAe3d( const std::string& aOutFile, VertexFormat vertexFormat )
{
    static_assert( sizeof( VertexPTNTC) == 64, "" );
    static_assert( sizeof( ae3d::Vec3 ) == 12, "" );
    static_assert( sizeof( VertexInd  ) ==  6, "" );

    if (gMeshes.empty())
    {
        std::cerr << "Model doesn't contain any meshes, aborting!" << std::endl;
        return;
    }
    
    std::ofstream ofs( aOutFile.c_str(), std::ios::binary );

    if (!ofs.is_open())
    {
        std::cerr << "Couldn't open file for writing!" << std::endl;
        exit( 1 );
    }

    for (std::size_t m = 0; m < gMeshes.size(); ++m)
    {
        gMeshes[ m ].SolveAABB();

        if (gMeshes[ m ].vnormal.empty())
        {
            gMeshes[ m ].SolveVertexNormals();
        }

        gMeshes[ m ].Interleave();
        gMeshes[ m ].OptimizeFaces();

        gMeshes[ m ].SolveFaceNormals();

        if (gMeshes[ m ].nonInterleavedTangents.empty())
        {
            gMeshes[ m ].SolveFaceTangents();
            gMeshes[ m ].SolveVertexTangents();
        }
    }

    // Calculates model's AABB by finding extreme values from meshes' AABBs.
    const float maxValue = 99999999.0f;
    ae3d::Vec3 aabbMin(  maxValue,  maxValue,  maxValue );
    ae3d::Vec3 aabbMax( -maxValue, -maxValue, -maxValue );

    for (std::size_t m = 0; m < gMeshes.size(); ++m)
    {
        aabbMin = ae3d::Vec3::Min2( aabbMin, gMeshes[ m ].aabbMin );
        aabbMax = ae3d::Vec3::Max2( aabbMax, gMeshes[ m ].aabbMax );
    }

    // The file starts with identification bytes.
    const char* gAe3dVersion = "a9";
    ofs.write( gAe3dVersion, 2 );

    ofs.write( reinterpret_cast< char* >( &aabbMin.x ), 3 * 4 );
    ofs.write( reinterpret_cast< char* >( &aabbMax.x ), 3 * 4 );

    const std::size_t meshes = gMeshes.size();
    // # of meshes.
    ofs.write( reinterpret_cast< const char* >( &meshes ), 2 );

    for (std::size_t m = 0; m < meshes; ++m)
    {
        assert( gMeshes[ m ].fnormal.size() == gMeshes[ m ].indices.size() );

        // AABB.
        ofs.write( reinterpret_cast< char* >( &gMeshes[ m ].aabbMin.x ), 3 * 4 );
        ofs.write( reinterpret_cast< char* >( &gMeshes[ m ].aabbMax.x ), 3 * 4 );

        // Writes name's length.
        const unsigned short nameLength = (unsigned short)gMeshes[ m ].name.length();
        ofs.write( reinterpret_cast< char* >((char*)&nameLength ), 2 );
        // Writes name.
        ofs.write(reinterpret_cast<char*>((char*)gMeshes[m].name.data()),
                  nameLength);
        
        // Writes # of vertices.
        const unsigned short nVertices = (unsigned short)gMeshes[ m ].interleavedVertices.size();
        ofs.write( reinterpret_cast< char* >((char*)&nVertices), 2 );

        // Writes vertex data.
        if (vertexFormat == VertexFormat::PTNTC_Skinned || !gMeshes[ m ].joints.empty())
        {
            const unsigned char format = 2;
            ofs.write( (char*)&format, 1 );

            ofs.write( (char*)&gMeshes[ m ].interleavedVertices[ 0 ], gMeshes[ m ].interleavedVertices.size() * sizeof( VertexPTNTC_Skinned ) );
        }
        else if (vertexFormat == VertexFormat::PTNTC)
        {
            gMeshes[ m ].CopyInterleavedVerticesToPTNTC();
            
            const unsigned char format = 0;
            ofs.write( (char*)&format, 1 );
            
            ofs.write( (char*)&gMeshes[ m ].interleavedVerticesPTNTC[ 0 ], gMeshes[ m ].interleavedVerticesPTNTC.size() * sizeof( VertexPTNTC ) );
        }
        else if (vertexFormat == VertexFormat::PTN)
        {
            gMeshes[ m ].CopyInterleavedVerticesToPTN();

            const unsigned char format = 1;
            ofs.write( (char*)&format, 1 );
        
            ofs.write( (char*)&gMeshes[m].interleavedVerticesPTN[ 0 ], gMeshes[ m ].interleavedVerticesPTN.size() * sizeof( VertexPTN ) );
        }
        else
        {
            std::cerr << "WriteAe3d: Unhandled Vertex format!" << std::endl;
            exit( 1 );
        }
        
        // Writes # of indices.
        const unsigned short indexCount = (unsigned short)gMeshes[m].indices.size();
        ofs.write( reinterpret_cast< char* >((char*)&indexCount), 2 );

        // Writes indices.
        ofs.write( (char*)&gMeshes[m].indices[ 0 ], gMeshes[m].indices.size() * sizeof( VertexInd ) );

        if (vertexFormat == VertexFormat::PTNTC_Skinned || !gMeshes[ m ].joints.empty())
        {
            // Writes # of joints.
            const unsigned short jointCount = (unsigned short)gMeshes[ m ].joints.size();
            ofs.write( reinterpret_cast< char* >((char*)&jointCount), 2 );
        
            // Writes joints.
            for (size_t j = 0; j < gMeshes[ m ].joints.size(); ++j)
            {
                ofs.write( (char*)&gMeshes[ m ].joints[ j ].globalBindposeInverse, sizeof( ae3d::Matrix44 ) );
                ofs.write( (char*)&gMeshes[ m ].joints[ j ].parentIndex, 4 );
                const int jointNameLength = (int)gMeshes[ m ].joints[ j ].name.length();
                ofs.write( (char*)&jointNameLength, sizeof( int ) );
                ofs.write( reinterpret_cast<char*>((char*)gMeshes[m].joints[ j ].name.data()), jointNameLength );
                const int animLength = (int)gMeshes[ m ].joints[ j ].animTransforms.size();
                ofs.write( (char*)&animLength, sizeof( int ) );
                ofs.write( (char*)&gMeshes[ m ].joints[ j ].animTransforms[ 0 ].m[ 0 ], gMeshes[ m ].joints[ j ].animTransforms.size() * sizeof( ae3d::Matrix44 ) );
            }
        }
    }

    // Terminator.
    const unsigned char byte = 100;
    ofs.write( (char*)&byte, 1 );

    std::cout << "Wrote " << aOutFile << std::endl;
}
#endif
