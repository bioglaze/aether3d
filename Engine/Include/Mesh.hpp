#ifndef MESH_H
#define MESH_H

#include <type_traits>
#include <vector>

namespace ae3d
{
    namespace FileSystem
    {
        struct FileContentsData;
    }

    struct SubMesh;

    /// Contains a mesh. Can contain submeshes.
    class Mesh
    {
      public:
        enum class LoadResult { Success, Corrupted, OutOfMemory };

        /// Constructor.
        Mesh();

        /// \param other Other.
        Mesh( const Mesh& other );
        
        /// Destructor.
        ~Mesh();

        /// \param other Other.
        Mesh& operator=( const Mesh& other );
        
        /// \param meshData Data from .ae3d mesh file.
        /// \return Load result.
        LoadResult Load( const FileSystem::FileContentsData& meshData );
        
      private:
        friend class MeshRendererComponent;
        
        struct Impl;
        Impl& m() { return reinterpret_cast<Impl&>(_storage); }
        Impl const& m() const { return reinterpret_cast<Impl const&>(_storage); }
        
        static const std::size_t StorageSize = 1384;
        static const std::size_t StorageAlign = 16;
        
        std::aligned_storage<StorageSize, StorageAlign>::type _storage;
        
        std::vector< SubMesh >& GetSubMeshes();
    };
}

#endif
