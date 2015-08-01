#include "Mesh.hpp"
#include <vector>
#include <sstream>
#include <cstdint>
#include <string>
#include "FileSystem.hpp"
#include "FileWatcher.hpp"
#include "VertexBuffer.hpp"
#include "SubMesh.hpp"
#include "System.hpp"

using namespace ae3d;

extern ae3d::FileWatcher fileWatcher;

void MeshReload( const std::string& path )
{
    System::Print("mesh reload unimplemented\n");
}

struct ae3d::Mesh::Impl
{
    Vec3 aabbMin;
    Vec3 aabbMax;
    std::vector< SubMesh > subMeshes;
};

struct MeshCacheEntry
{
    std::string path;
    Vec3 aabbMin;
    Vec3 aabbMax;
    std::vector< SubMesh > subMeshes;    
};

std::vector< MeshCacheEntry > gMeshCache;

ae3d::Mesh::Mesh()
{
    new(&_storage)Impl();
}

ae3d::Mesh::~Mesh()
{
    reinterpret_cast< Impl* >(&_storage)->~Impl();
}

ae3d::Mesh::Mesh( const Mesh& other )
{
    new(&_storage)Impl();
    reinterpret_cast<Impl&>(_storage) = reinterpret_cast<Impl const&>(other._storage);
}

ae3d::Mesh& ae3d::Mesh::operator=( const Mesh& other )
{
    new(&_storage)Impl();
    reinterpret_cast<Impl&>(_storage) = reinterpret_cast<Impl const&>(other._storage);
    return *this;
}

const Vec3& ae3d::Mesh::GetAABBMin() const
{
    return m().aabbMin;
}

const Vec3& ae3d::Mesh::GetAABBMax() const
{
    return m().aabbMax;
}

const Vec3& ae3d::Mesh::GetSubMeshAABBMin( unsigned subMeshIndex ) const
{
    return m().subMeshes[ subMeshIndex < m().subMeshes.size() ? subMeshIndex : 0 ].aabbMin;
}

const Vec3& ae3d::Mesh::GetSubMeshAABBMax( unsigned subMeshIndex ) const
{
    return m().subMeshes[ subMeshIndex < m().subMeshes.size() ? subMeshIndex : 0 ].aabbMax;
}

std::vector< ae3d::SubMesh >& ae3d::Mesh::GetSubMeshes()
{
    return m().subMeshes;
}

ae3d::Mesh::LoadResult ae3d::Mesh::Load( const FileSystem::FileContentsData& meshData )
{
    for (const auto& entry : gMeshCache)
    {
        if (entry.path == meshData.path)
        {
            System::Print("Loading from cache: %s\n", entry.path.c_str());
            m().aabbMin = entry.aabbMin;
            m().aabbMax = entry.aabbMax;
            m().subMeshes = entry.subMeshes;
            return LoadResult::Success;
        }
    }
    
    if (!meshData.isLoaded)
    {
        const float s = 5;
        
        const std::vector< VertexBuffer::VertexPTC > vertices =
        {
            { Vec3( -s, -s, s ), 0, 0 },
            { Vec3( s, -s, s ), 0, 0 },
            { Vec3( s, -s, -s ), 0, 0 },
            { Vec3( -s, -s, -s ), 0, 0 },
            { Vec3( -s, s, s ), 0, 0 },
            { Vec3( s, s, s ), 0, 0 },
            { Vec3( s, s, -s ), 0, 0 },
            { Vec3( -s, s, -s ), 0, 0 }
        };
        
        const std::vector< VertexBuffer::Face > indices =
        {
            { 0, 4, 1 },
            { 4, 5, 1 },
            { 1, 5, 2 },
            { 2, 5, 6 },
            { 2, 6, 3 },
            { 3, 6, 7 },
            { 3, 7, 0 },
            { 0, 7, 4 },
            { 4, 7, 5 },
            { 5, 7, 6 },
            { 3, 0, 2 },
            { 2, 0, 1 }
        };
        
        m().subMeshes.resize( 1 );
        m().subMeshes[ 0 ].vertexBuffer.Generate( indices.data(), static_cast< int >( indices.size() ), vertices.data(), static_cast< int >( vertices.size() ) );
        m().subMeshes[ 0 ].aabbMin = { -s, -s, -s };
        m().subMeshes[ 0 ].aabbMax = {  s,  s,  s };
        return LoadResult::FileNotFound;
    }
    
    uint8_t magic[ 2 ];

    std::istringstream is( std::string( std::begin( meshData.data ), std::end( meshData.data ) ) );

    is.read( (char*)magic, sizeof( magic ) );

    if (magic[ 0 ] != 'a' || magic[ 1 ] != '9')
    {
        System::Print( "%s is corrupted or old format: Wrong magic number!\n", meshData.path.c_str() );
        return LoadResult::Corrupted;
    }

    is.read( (char*)&m().aabbMin, sizeof( m().aabbMin ) );
    is.read( (char*)&m().aabbMax, sizeof( m().aabbMax ) );

    if (m().aabbMin.x > m().aabbMax.x || m().aabbMin.y > m().aabbMax.y || m().aabbMin.z > m().aabbMax.z)
    {
        return LoadResult::Corrupted;
    }
    
    uint16_t meshCount;
    is.read( (char*)&meshCount, sizeof( meshCount ) );

    m().subMeshes.clear();
    m().subMeshes.resize( meshCount );

    for (auto& subMesh : m().subMeshes)
    {
        is.read( (char*)&subMesh.aabbMin, sizeof( subMesh.aabbMin ) );
        is.read( (char*)&subMesh.aabbMax, sizeof( subMesh.aabbMax ) );

        uint16_t nameLength;
        is.read( (char*)&nameLength, sizeof( nameLength ) );

        std::vector< char > meshName( nameLength + 1 );
        is.read( (char*)&meshName[ 0 ], nameLength );
        subMesh.name = std::string( meshName.data(), meshName.size() - 1 );

        uint16_t vertexCount;
        is.read( (char*)&vertexCount, sizeof( vertexCount ) );

        std::vector< VertexBuffer::VertexPTNTC > verticesPTNTC;
        std::vector< VertexBuffer::VertexPTN > verticesPTN;

        uint8_t vertexFormat;
        is.read( (char*)&vertexFormat, sizeof( vertexFormat ) );
        
        if (vertexFormat != 0 && vertexFormat != 1)
        {
            System::Print( "Mesh %s submesh %s has invalid vertex format %d. Only 0 and 1 are valid!\n", meshData.path.c_str(), subMesh.name.c_str(), vertexFormat );
            return LoadResult::Corrupted;
        }
        else if (vertexFormat == 0) // PTNTC
        {
            try { verticesPTNTC.resize( vertexCount ); }
            catch (std::bad_alloc&)
            {
                return LoadResult::OutOfMemory;
            }
        
            is.read( (char*)&verticesPTNTC[ 0 ].position.x, vertexCount * sizeof( VertexBuffer::VertexPTNTC ) );
        }
        else if (vertexFormat == 1) // PTN
        {
            try { verticesPTN.resize( vertexCount ); }
            catch (std::bad_alloc&)
            {
                return LoadResult::OutOfMemory;
            }
            
            is.read( (char*)&verticesPTN[ 0 ].position.x, vertexCount * sizeof( VertexBuffer::VertexPTN ) );
        }
        
        uint16_t faceCount;
        is.read( (char*)&faceCount, sizeof( faceCount ) );

        std::vector< VertexBuffer::Face > indices;

        try { indices.resize( faceCount ); }
        catch (std::bad_alloc&)
        {
            return LoadResult::OutOfMemory;
        }

        is.read( (char*)&indices[ 0 ], faceCount * sizeof( VertexBuffer::Face ) );

        if (vertexFormat == 0)
        {
            subMesh.vertexBuffer.Generate( indices.data(), static_cast< int >( indices.size() ), verticesPTNTC.data(), static_cast< int >( verticesPTNTC.size() ) );
        }
        else if (vertexFormat == 1)
        {
            subMesh.vertexBuffer.Generate( indices.data(), static_cast< int >( indices.size() ), verticesPTN.data(), static_cast< int >( verticesPTN.size() ) );
        }
    }
    
    uint8_t terminator;
    is.read( (char*)&terminator, sizeof( terminator ) );

    if (terminator != 100)
    {
        return LoadResult::Corrupted;
    }

    MeshCacheEntry cacheEntry;
    cacheEntry.path = meshData.path;
    cacheEntry.aabbMin = m().aabbMin;
    cacheEntry.aabbMax = m().aabbMax;
    cacheEntry.subMeshes = m().subMeshes;
    gMeshCache.push_back( cacheEntry );

    fileWatcher.AddFile( meshData.path, MeshReload );
    
    return LoadResult::Success;
}
