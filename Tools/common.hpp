#ifndef COMMON_H
#define COMMON_H

#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "Matrix.hpp"
#include "Vec3.hpp"

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
        vInd[0] = vInd[1] = vInd[2] = 0;
        uvInd[0] = uvInd[1] = uvInd[2] = 0;
        vnInd[0] = vnInd[1] = vnInd[2] = 0;
        colInd[0] = colInd[1] = colInd[2] = 0;
    }

    unsigned short vInd[3];
    unsigned short uvInd[3];
    unsigned short vnInd[3];
    unsigned short colInd[3];
};

// Defines a face used in a vertex array.
// a, b and c are indices to Mesh::interleavedVertices.
struct VertexInd
{
    unsigned short a, b, c;
};

enum class VertexFormat { PTNTC, PTN };

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

struct Mesh
{
    void BuildVertexInfluences();
    void Interleave();
    void SolveAABB();
    void SolveFaceNormals();
    void SolveFaceTangents();
    void SolveVertexNormals();
    void SolveVertexTangents();
    void CopyInterleavedVerticesToPTN();

    bool AlmostEquals( const ae3d::Vec3& v1, const ae3d::Vec3& v2 ) const;
    bool AlmostEquals( const ae3d::Vec4& v1, const ae3d::Vec4& v2 ) const;
    bool AlmostEquals( const TexCoord& t1, const TexCoord& t2 ) const;

    std::string name = { "unnamed" };
    std::vector< ae3d::Vec3 > vertex;
    std::vector< ae3d::Vec4 > tangents;
    std::vector< ae3d::Vec3 > vnormal;
    std::vector< ae3d::Vec4 > colors;
    std::vector< ae3d::Vec3 > fnormal;
    std::vector< TexCoord > tcoord;
    std::vector< Face >     face;

    // Separate element arrays combined to one.
    // These are written to the .ae3d file.
    std::vector< VertexPTNTC > interleavedVertices;
    std::vector< VertexPTN > interleavedVerticesPTN;
    std::vector< VertexInd > indices;

    // Used to calculate tangent-space handedness.
    std::vector< ae3d::Vec3 > bitangents;  // For faces.
    std::vector< ae3d::Vec3 > vbitangents; // For vertices.

    ae3d::Vec3 aabbMax;
    ae3d::Vec3 aabbMin;
};

std::vector< Mesh > gMeshes;

void Mesh::CopyInterleavedVerticesToPTN()
{
    interleavedVerticesPTN.resize( interleavedVertices.size() );
    
    for (std::size_t i = 0; i < interleavedVertices.size(); ++i)
    {
        interleavedVerticesPTN[ i ].position = interleavedVertices[ i ].position;
        interleavedVerticesPTN[ i ].texCoord = interleavedVertices[ i ].texCoord;
        interleavedVerticesPTN[ i ].normal = interleavedVertices[ i ].normal;
    }
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

    vbitangents.resize( interleavedVertices.size() );

    // http://www.terathon.com/code/tangent.html
    // "To find the tangent vectors for a single vertex, we average the tangents for all triangles sharing that vertex"
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
        tangent.w = ae3d::Vec3::Dot( cp, vbitangents[v] ) >= 0 ? 1 : -1;
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

        // Searches face f's vertex a from the vertex list.
        // If it's not found, it's added.
        bool found = false;
        
        for (std::size_t i = 0; i < indices.size(); ++i)
        {
            if (AlmostEquals( interleavedVertices[ indices[ i ].a ].position, tvertex ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].a ].normal,   tnormal ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].a ].texCoord, ttcoord ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].a ].color, tcolor ))
            {
                found = true;
                newFace.a = indices[ i ].a;
                break;
            }
        }

        if (!found)
        {
            VertexPTNTC newVertex;
            newVertex.position = tvertex;
            newVertex.normal   = tnormal;
            newVertex.texCoord = ttcoord;
            newVertex.color    = tcolor;

            interleavedVertices.push_back( newVertex );

            newFace.a = (unsigned short)(interleavedVertices.size() - 1);
        }

        // vertind 1
        tvertex = vertex [ face[ f ].vInd [ 1 ] ];
        tnormal = vnormal[ face[ f ].vnInd[ 1 ] ];
        ttcoord = tcoord.empty() ? TexCoord() : tcoord [ face[ f ].uvInd[ 1 ] ];
        tcolor = colors.empty() ? ae3d::Vec4( 0, 0, 0, 1 ) : colors[ face[ f ].colInd[ 1 ] ];
        
        found = false;

        for (std::size_t i = 0; i < indices.size(); ++i)
        {
            if (AlmostEquals( interleavedVertices[ indices[ i ].b ].position, tvertex ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].b ].normal,   tnormal ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].b ].texCoord, ttcoord ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].b ].color, tcolor ))
                
            {
                found = true;

                newFace.b = indices[ i ].b;
                break;
            }
        }

        if (!found)
        {
            VertexPTNTC newVertex;
            newVertex.position = tvertex;
            newVertex.normal   = tnormal;
            newVertex.texCoord = ttcoord;
            newVertex.color    = tcolor;

            interleavedVertices.push_back( newVertex );

            newFace.b = (unsigned short)(interleavedVertices.size() - 1);
        }

        // vertind 2
        tvertex = vertex [ face[ f ].vInd [ 2 ] ];
        tnormal = vnormal[ face[ f ].vnInd[ 2 ] ];
        ttcoord = tcoord.empty() ? TexCoord() :  tcoord [ face[ f ].uvInd[ 2 ] ];
        tcolor = colors.empty() ? ae3d::Vec4( 0, 0, 0, 1 ) : colors[ face[ f ].colInd[ 2 ] ];

        found = false;

        for (std::size_t i = 0; i < indices.size(); ++i)
        {
            if (AlmostEquals( interleavedVertices[ indices[ i ].c ].position, tvertex ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].c ].normal,   tnormal ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].c ].texCoord, ttcoord ) &&
                AlmostEquals( interleavedVertices[ indices[ i ].c ].color, tcolor ))
            {
                found = true;

                newFace.c = indices[ i ].c;
                break;
            }
        }

        if (!found)
        {
            VertexPTNTC newVertex;
            newVertex.position = tvertex;
            newVertex.normal   = tnormal;
            newVertex.texCoord = ttcoord;
            newVertex.color    = tcolor;

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

        gMeshes[ m ].SolveFaceNormals();
        gMeshes[ m ].SolveFaceTangents();
        gMeshes[ m ].SolveVertexTangents();
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
        if (vertexFormat == VertexFormat::PTNTC)
        {
            const unsigned char format = 0;
            ofs.write( (char*)&format, 1 );

            ofs.write( (char*)&gMeshes[m].interleavedVertices[ 0 ], gMeshes[m].interleavedVertices.size() * sizeof( VertexPTNTC ) );
        }
        else if (vertexFormat == VertexFormat::PTN)
        {
            gMeshes[ m ].CopyInterleavedVerticesToPTN();

            const unsigned char format = 1;
            ofs.write( (char*)&format, 1 );
        
            ofs.write( (char*)&gMeshes[m].interleavedVerticesPTN[ 0 ], gMeshes[m].interleavedVerticesPTN.size() * sizeof( VertexPTN ) );
        }
        else
        {
            std::cerr << "WriteAe3d: Unhandled Vertex format!" << std::endl;
            exit( 1 );
        }
        
        // Writes # of indices.
        const unsigned short nIndices = (unsigned short)gMeshes[m].indices.size();
        ofs.write( reinterpret_cast< char* >((char*)&nIndices), 2 );

        // Writes indices.
        ofs.write( (char*)&gMeshes[m].indices[ 0 ], gMeshes[m].indices.size() * sizeof( VertexInd ) );
    }

    // Terminator.
    const unsigned char byte = 100;
    ofs.write( (char*)&byte, 1 );

    std::cout << "Wrote " << aOutFile << std::endl;
}

#endif
