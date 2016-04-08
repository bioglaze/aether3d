#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

#include <string>
#include "Vec3.hpp"
#include "Matrix.hpp"
#include "RenderTexture.hpp"

namespace ae3d
{
    /// Camera views the scene. Game Object containing a camera component must also contain a transform component to render anything.
    class CameraComponent
    {
    public:
        /// Projection type
        enum class ProjectionType { Orthographic, Perspective };
        
        /// Clear flag.
        enum class ClearFlag { DepthAndColor, Depth, DontClear };
        
        /// \return Projection matrix.
        const Matrix44& GetProjection() const { return projectionMatrix; }

        /// \return View matrix.
        const Matrix44& GetView() const { return viewMatrix; }

        /// \return Projection type.
        ProjectionType GetProjectionType() const { return projectionType; }

        /// \param aProjectionType Projection type. Defaults to orthographic.
        void SetProjectionType( ProjectionType aProjectionType ) { projectionType = aProjectionType; }
        
        /**
          Sets an orthographic projection matrix.
         
         \param left Left.
         \param right Right.
         \param bottom Bottom.
         \param top Top.
         \param near Near plane distance.
         \param far Far plane distance.
         */
        void SetProjection( float left, float right, float bottom, float top, float near, float far );

        /**
         Sets a perspective projection matrix.
         
         \param fovDegrees Field of view in degrees.
         \param aspect Aspect ratio.
         \param near Near plane distance.
         \param far Far plane distance.
         */
        void SetProjection( float fovDegrees, float aspect, float near, float far );

        /// \param proj Projection matrix.
        void SetProjection( const Matrix44& proj );

        /// \return Clear color in range 0-1.
        Vec3 GetClearColor() const { return clearColor; }

        /// \return Target texture or null if there is no target texture.
        RenderTexture* GetTargetTexture() { return targetTexture; }

        /// \return Depth and normals texture.
        RenderTexture& GetDepthNormalsTexture() { return depthNormalsTexture; }

        /// \param color Color in range 0-1.
        void SetClearColor( const Vec3& color );

        /// \param renderTexture 2D or Cube render texture.
        void SetTargetTexture( RenderTexture* renderTexture );

        /// \param aClearFlag Clear flag. Defaults to DepthAndColor.
        void SetClearFlag( ClearFlag aClearFlag ) { clearFlag = aClearFlag; }
        
        /// \param aLayerMask Layer mask contains OR'd layers that this camera renders into.
        void SetLayerMask( unsigned aLayerMask ) { layerMask = aLayerMask; }
        
        /// \return Render order.
        unsigned GetRenderOrder() const { return renderOrder; }

        /// \param order Order. Higher values are rendered after lower values.
        void SetRenderOrder( unsigned order ) { renderOrder = order; }
        
        /// \return Layer mask.
        unsigned GetLayerMask() const { return layerMask; }
        
        /// \return Clear flag.
        ClearFlag GetClearFlag() const { return clearFlag; }
        
        /// \return Near plane.
        float GetNear() const { return nearp; }

        /// \return Far plane.
        float GetFar() const { return farp; }

        /// \return Aspect ratio.
        float GetAspect() const { return aspect; }

        /// \return FOV in degrees.
        float GetFovDegrees() const { return fovDegrees; }

        /// \return Textual representation of component.
        std::string GetSerialized() const;

        /// \return Left clipping plane.
        float GetLeft() const { return orthoParams.left; }

        /// \return Right clipping plane.
        float GetRight() const { return orthoParams.right; }

        /// \return Bottom clipping plane.
        float GetBottom() const { return orthoParams.down; }

        /// \return Top clipping plane.
        float GetTop() const { return orthoParams.top; }

        /// TODO: Convert to private
        /// \param view View matrix.
        void SetView( const Matrix44& view ) { viewMatrix = view; }

    private:
        friend class GameObject;
        friend class Scene;

        /// \return Component's type code. Must be unique for each component type.
        static int Type() { return 0; }
        
        /// \return Component handle that uniquely identifies the instance.
        static unsigned New();
        
        /// \return Component at index or null if index is invalid.
        static CameraComponent* Get( unsigned index );

        Matrix44 projectionMatrix;
        Matrix44 viewMatrix;
        Vec3 clearColor;
        RenderTexture* targetTexture = nullptr;
        RenderTexture depthNormalsTexture;

        struct OrthoParams
        {
            float left = 0;
            float right = 100;
            float top = 0;
            float down = 100;
        } orthoParams;

        float nearp = 0;
        float farp = 1;
        float fovDegrees = 45;
        float aspect = 1;
        unsigned layerMask = 1;
        unsigned renderOrder = 0;
        ProjectionType projectionType = ProjectionType::Orthographic;
        ClearFlag clearFlag = ClearFlag::DepthAndColor;
    };
}
#endif
