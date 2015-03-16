#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

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

        void SetProjection( int left, int right, int bottom, int top, float near, float far );

    private:
    };
}
#endif
