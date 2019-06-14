#pragma once

namespace ae3d
{
    struct Vec3;
    class GameObject;

    namespace VR
    {
        /// Inits the device. Must be called as early as possible.
        void Init();

        /// Releases resources.
        void Deinit();

        /// \return Left hand position
        Vec3 GetLeftHandPosition();

        /// \return Right hand position
        Vec3 GetRightHandPosition();

        /// \param outWidth Returns optimal width for window.
        /// \param outHeight Returns optimal height for window.
        void GetIdealWindowSize( int& outWidth, int& outHeight );

        /// Displays the image on the device. Called after both eyes have been rendered.
        void SubmitFrame();

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
