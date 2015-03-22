#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

#include "Vec3.hpp"
#include "Matrix.hpp"

namespace ae3d
{
    /**
      Camera views the scene.
    */
    class CameraComponent
    {
    public:
        // \return Component's type code. Must be unique for each component type.
        static int Type() { return 0; }

        // \return Component handle that uniquely identifies the instance.
        static int New();

        // \return Component at index or null if index is invalid.
        static CameraComponent* Get(int index);

        // \return Projection matrix.
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
        void SetProjection( int left, int right, int bottom, int top, float near, float far );

        // \return clear color.
        Vec3 GetClearColor() const { return clearColor; }

        // \param color Color in range 0-1.
        void SetClearColor( const Vec3& color );

    private:
        static ae3d::CameraComponent cameraComponents[100];
        Matrix44 projectionMatrix;
        Matrix44 viewMatrix;
        Vec3 clearColor;
    };
}
#endif
