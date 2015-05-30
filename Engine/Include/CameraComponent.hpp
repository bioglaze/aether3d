#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

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

        /// \return Clear color in range 0-1.
        Vec3 GetClearColor() const { return clearColor; }

        /// \return Target texture or null if there is no target texture.
        RenderTexture2D* GetTargetTexture() { return targetTexture; }
        
        /// \param color Color in range 0-1.
        void SetClearColor( const Vec3& color );

        /// \param renderTexture2D 2D render texture.
        void SetTargetTexture( RenderTexture2D* renderTexture2D );
        
    private:
        friend class GameObject;
        
        /** \return Component's type code. Must be unique for each component type. */
        static int Type() { return 0; }
        
        /** \return Component handle that uniquely identifies the instance. */
        static unsigned New();
        
        /** \return Component at index or null if index is invalid. */
        static CameraComponent* Get( unsigned index );

        Matrix44 projectionMatrix;
        Matrix44 viewMatrix;
        Vec3 clearColor;
        RenderTexture2D* targetTexture = nullptr;
    };
}
#endif
