#ifndef MESH_H
#define MESH_H

#include <type_traits>
#include <vector>
#include <string>

namespace ae3d
{
    namespace FileSystem
    {
        struct FileContentsData;
    }

    struct SubMesh;
    struct Vec3;
    
    /// Contains a mesh. Can contain submeshes.
    class Mesh
    {
      public:
        /// Result of loading the mesh.
        enum class LoadResult { Success, Corrupted, OutOfMemory, FileNotFound };

        /// Constructor.
        Mesh();

        /// \param other Other.
        Mesh( const Mesh& other );
        
        /// Destructor.
        ~Mesh();

        /// \param other Other.
        Mesh& operator=( const Mesh& other );

        /// \return Path where this mesh was loaded from.
        const std::string& GetPath() const;
        
        /// \param meshData Data from .ae3d mesh file.
        /// \return Load result.
        LoadResult Load( const FileSystem::FileContentsData& meshData );
        
        /// \return Axis-aligned bounding box minimum in local coordinates.
        const Vec3& GetAABBMin() const;
        
        /// \return Axis-aligned bounding box maximum in local coordinates.
        const Vec3& GetAABBMax() const;

        /// \param subMeshIndex Submesh index. If invalid, the first submesh AABB min is returned.
        /// \return Axis-aligned bounding box minimum for a submesh in local coordinates.
        const Vec3& GetSubMeshAABBMin( unsigned subMeshIndex ) const;
        
        /// \param subMeshIndex Submesh index. If invalid, the first submesh AABB max is returned.
        /// \return Axis-aligned bounding box maximum in local coordinates.
        const Vec3& GetSubMeshAABBMax( unsigned subMeshIndex ) const;

        /// \return Submesh triangles
        std::vector< Vec3 > GetSubMeshFlattenedTriangles( unsigned subMeshIndex ) const;
        
        /// \return Submesh count.
        unsigned GetSubMeshCount() const;
        
        /// \param index Submesh index.
        /// \return Submesh name. If index is invalid, returns first submesh's name.
        const std::string& GetSubMeshName( unsigned index ) const;
        
      private:
        friend class MeshRendererComponent;
        
        struct Impl;
        Impl& m() { return reinterpret_cast<Impl&>(_storage); }
        Impl const& m() const { return reinterpret_cast<Impl const&>(_storage); }
        
        static const std::size_t StorageSize = 1384;
        static const std::size_t StorageAlign = 16;
        
        std::aligned_storage<StorageSize, StorageAlign>::type _storage = {};
        
        std::vector< SubMesh >& GetSubMeshes();
    };
}

#endif
