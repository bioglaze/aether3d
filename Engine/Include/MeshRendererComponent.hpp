#pragma once

#include "Array.hpp"

namespace ae3d
{
    /// Contains a Mesh. The game object must also contain a TransformComponent to be able to render the mesh.
    class MeshRendererComponent
    {
    public:
        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }

        /// \return True, if the object casts shadow.
        bool CastsShadow() const { return castShadow; }
        
        /// \param enabled True, if the object casts shadow.
        void SetCastShadow( bool enabled ) { castShadow = enabled; }

        /// \return True, if the component is enabled.
        bool IsEnabled() const { return isEnabled; }
        
        /// \param enabled True if the component should be rendered, false otherwise.
        void SetEnabled( bool enabled ) { isEnabled = enabled; }

        /// \return Mesh.
        class Mesh* GetMesh() { return mesh; }
        
        /// \param subMeshIndex Sub mesh index.
        /// \return Submesh's material or null if the index is invalid.
        class Material* GetMaterial( int subMeshIndex );

        /// \param material Material.
        /// \param subMeshIndex Sub mesh index.
        void SetMaterial( Material* material, unsigned subMeshIndex );

        /// \param aMesh Mesh.
        void SetMesh( Mesh* aMesh );

        /// \return True, if bounding box should be drawn.
        bool IsBoundingBoxDrawingEnabled() const;
        
        /// \param enable True, if AABB will be rendered. Defaults to false.
        void EnableBoundingBoxDrawing( bool enable );
        
        /// \param frame Animation frame. If too high or low, repeats from the beginning using modulo.
        void SetAnimationFrame( int frame ) { animFrame = frame; }
        
        /// \return True, if the mesh will be rendered as a wireframe.
        bool IsWireframe() const { return isWireframe; }

        /// \param enable True, if the mesh will be rendered as a wireframe.
        void EnableWireframe( bool enable ) { isWireframe = enable; }
        
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
        
        /// Applies skin
        /// \param subMeshIndex Submesh index
        void ApplySkin( unsigned subMeshIndex );
        
        /// \param cameraFrustum cameraFrustum
        /// \param localToWorld Local-to-World matrix
        void Cull( const class Frustum& cameraFrustum, const struct Matrix44& localToWorld );
        
        /// \param localToView Model-view matrix.
        /// \param localToClip Model-view-projection matrix.
        /// \param localToWorld Transforms mesh AABB from mesh-local space into world-space.
        /// \param shadowView Shadow camera view matrix.
        /// \param shadowProjection Shadow camera projection matrix.
        /// \param overrideShader Override shader. Used for shadow pass.
        /// \param overrideSkinShader Override shader for skinned meshes. Used for shadow pass.
        /// \param renderType Renderer type.
        void Render( const struct Matrix44& localToView, const Matrix44& localToClip, const Matrix44& localToWorld,
                     const Matrix44& shadowView, const Matrix44& shadowProjection, class Shader* overrideShader,
                     Shader* overrideSkinShader, RenderType renderType );

        Mesh* mesh = nullptr;
        Array< Material* > materials;
        Array< bool > isSubMeshCulled;
        GameObject* gameObject = nullptr;
        int animFrame = 0;
        bool isCulled = false;
        bool isWireframe = false;
        bool isEnabled = true;
        bool castShadow = true;
        bool isAabbDrawingEnabled = false;
        int aabbLineHandle = -1;
    };
}
