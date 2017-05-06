#ifndef MESH_RENDERER_COMPONENT
#define MESH_RENDERER_COMPONENT

#include <string>
#include <vector>

namespace ae3d
{
    /// Contains a Mesh. The game object must also contain a TransformComponent to be able to render the mesh.
    class MeshRendererComponent
    {
    public:
        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }

        /// \param enabled True if the component should be rendered, false otherwise.
        void SetEnabled( bool enabled ) { isEnabled = enabled; }

        /// \return Mesh.
        class Mesh* GetMesh() { return mesh; }
        
        /// \param material Material.
        /// \param subMeshIndex Sub mesh index.
        void SetMaterial( class Material* material, int subMeshIndex );

        /// \param aMesh Mesh.
        void SetMesh( Mesh* aMesh );

        /// \return True, if the mesh will be rendered as a wireframe.
        bool IsWireframe() const { return isWireframe; }

        /// \param enable True, if the mesh will be rendered as a wireframe.
        void EnableWireframe( bool enable ) { isWireframe = enable; }

        /// \return Textual representation of component.
        std::string GetSerialized() const;
        
    private:
        friend class GameObject;
        friend class Scene;
        
        enum class RenderType { Opaque, Transparent };
        
        /// \return Component's type code. Must be unique for each component type.
        static int Type() { return 5; }
        
        /// \return Component handle that uniquely identifies the instance.
        static unsigned New();
        
        /// \return Component at index or null if index is invalid.
        static MeshRendererComponent* Get( unsigned index );
        
        /// \param cameraFrustum cameraFrustum
        /// \param localToWorld Local-to-World matrix
        void Cull( const class Frustum& cameraFrustum, const struct Matrix44& localToWorld );
        
        /// \param modelView Model-view matrix.
        /// \param modelViewProjectionMatrix Model-view-projection matrix.
        /// \param localToWorld Transforms mesh AABB from mesh-local space into world-space.
        void Render( const struct Matrix44& modelView, const Matrix44& modelViewProjectionMatrix, const Matrix44& localToWorld,
                     const Matrix44& shadowView, const Matrix44& shadowProjection, class Shader* overrideShader, RenderType renderType );

        Mesh* mesh = nullptr;
        std::vector< Material* > materials;
        std::vector< bool > isSubMeshCulled;
        GameObject* gameObject = nullptr;
        bool isCulled = false;
        bool isWireframe = false;
        bool isEnabled = true;
    };
}

#endif
