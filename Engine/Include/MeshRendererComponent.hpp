#ifndef MESH_RENDERER_COMPONENT
#define MESH_RENDERER_COMPONENT

#include <string>

namespace ae3d
{
    class Mesh;
    
    /// Contains a mesh.
    class MeshRendererComponent
    {
    public:
        /// \param aMesh Mesh.
        void SetMesh( Mesh* aMesh ) { mesh = aMesh; }
        
        /// \return Textual representation of component.
        std::string GetSerialized() const;
        
    private:
        friend class GameObject;
        friend class Scene;
        
        /// \return Component's type code. Must be unique for each component type.
        static int Type() { return 5; }
        
        /// \return Component handle that uniquely identifies the instance.
        static unsigned New();
        
        /// \return Component at index or null if index is invalid.
        static MeshRendererComponent* Get( unsigned index );
        
        /* \param projectionModelMatrix Projection and model matrix combined. */
        void Render( const float* projectionModelMatrix );

        Mesh* mesh = nullptr;
    };
}

#endif
