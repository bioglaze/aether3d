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

    class SubMesh;

    /// Contains a mesh. Can contain submeshes.
    class Mesh
    {
      public:
        /// Constructor.
        Mesh();
        
        /// Destructor.
        ~Mesh();
        
        /// \param meshData Data from .ae3d mesh file.
        void Load( const FileSystem::FileContentsData& meshData );
        
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
