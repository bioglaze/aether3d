#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

#include <string>
#include "Vec3.hpp"
#include "Matrix.hpp"

namespace ae3d
{
    class RenderTexture2D;

    /// Camera views the scene.
    class CameraComponent
    {
    public:
        /// \return Projection matrix.
        const Matrix44& GetProjection() const { return projectionMatrix; }

        /// \return View matrix.
        const Matrix44& GetView() const { return viewMatrix; }

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

        /// \return Clear color in range 0-1.
        Vec3 GetClearColor() const { return clearColor; }

        /// \return Target texture or null if there is no target texture.
        RenderTexture2D* GetTargetTexture() { return targetTexture; }
        
        /// \param color Color in range 0-1.
        void SetClearColor( const Vec3& color );

        /// \param renderTexture2D 2D render texture.
        void SetTargetTexture( RenderTexture2D* renderTexture2D );
        
        /// \return Textual representation of component.
        std::string GetSerialized() const;

    private:
        friend class GameObject;
        friend class Scene;

        /// \return Component's type code. Must be unique for each component type.
        static int Type() { return 0; }
        
        /// \return Component handle that uniquely identifies the instance.
        static unsigned New();
        
        /// \return Component at index or null if index is invalid.
        static CameraComponent* Get( unsigned index );

        /// \param view View matrix.
        void SetView( const Matrix44& view ) { viewMatrix = view; }

        Matrix44 projectionMatrix;
        Matrix44 viewMatrix;
        Vec3 clearColor;
        RenderTexture2D* targetTexture = nullptr;
        
        struct OrthoParams
        {
            float left = 0;
            float right = 100;
            float top = 0;
            float down = 100;
        } orthoParams;

        float nearp = 0;
        float farp = 1;
    };
}
#endif
