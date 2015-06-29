#include "Mesh.hpp"
#include <vector>
#include "FileSystem.hpp"
#include "VertexBuffer.hpp"
#include "SubMesh.hpp"

struct ae3d::Mesh::Impl
{
    std::vector< SubMesh > subMeshes;
};

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

std::vector< ae3d::SubMesh >& ae3d::Mesh::GetSubMeshes()
{
    return m().subMeshes;
}

void ae3d::Mesh::Load( const FileSystem::FileContentsData& meshData )
{
    // TODO: Load from file data.
    const float s = 25;
    
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
}
