#ifndef MESH_RENDERER_COMPONENT
#define MESH_RENDERER_COMPONENT

#include <string>
#include <vector>

namespace ae3d
{
    /// Contains a Mesh.
    class MeshRendererComponent
    {
    public:
        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }

        /// \return Mesh.
        class Mesh* GetMesh() { return mesh; }
        
        /// \param material Material.
        /// \param subMeshIndex Sub mesh index.
        void SetMaterial( class Material* material, int subMeshIndex );

        /// \param aMesh Mesh.
        void SetMesh( Mesh* aMesh );

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
        
        /// \param modelView Model-view matrix.
        /// \param modelViewProjectionMatrix Model-view-projection matrix.
        /// \param cameraFrustum Camera frustum.
        /// \param localToWorld Transforms mesh AABB from mesh-local space into world-space.
        void Render( const struct Matrix44& modelView, const Matrix44& modelViewProjectionMatrix, const class Frustum& cameraFrustum, const Matrix44& localToWorld, class Shader* overrideShader );

        Mesh* mesh = nullptr;
        std::vector< Material* > materials;
        GameObject* gameObject = nullptr;
    };
}

#endif
