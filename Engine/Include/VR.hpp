#ifndef VR_H
#define VR_H

namespace ae3d
{
    class GameObject;

    namespace VR
    {
        /// Inits the device. Must be called as early as possible.
        void Init();

        /// \param outWidth Returns optimal width for window.
        /// \param outHeight Returns optimal height for window.
        void GetIdealWindowSize( int& outWidth, int& outHeight );

        /// Creates mirror textures and starts tracking the head pose.
        /// \param windowWidth Window width in pixels.
        /// \param windowHeight Window height in pixels.
        void StartTracking( int windowWidth, int windowHeight );

        /// Displays the image on the device. Called after both eyes have been rendered.
        void SubmitFrame();

        /// Sets active eye. Called before scene.Render().
        /// \param eye 0 for left eye, 1 for right.
        void SetEye( int eye );

        /// Unsets active eye. Called after scene.Render().
        /// \param eye 0 for left eye, 1 for right.
        void UnsetEye( int eye );

        /// Gets the pose from device.
        void CalcEyePose();

        /// Sets camera's projection and view matrix according to device.
        /// \param camera Camera. Must contain TransformComponent and CameraComponent.
        /// \param yawDegrees Yaw in degrees.
        /// \param eye 0 for left eye, 1 for right.
        void CalcCameraForEye( GameObject& camera, float yawDegrees, int eye );

        /// Re-centers the sensor position and orientation.
        void RecenterTracking();
    }
}

#endif
