#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include <string>
#include "Vec3.hpp"
#include "Matrix.hpp"
#include "Quaternion.hpp"

namespace ae3d
{
    /// Stores a position and an orientation.
    class TransformComponent
    {
    public:
        /// \return Local position.
        const Vec3& GetLocalPosition() const { return localPosition; }

        /// \return Local rotation.
        const Quaternion& GetLocalRotation() const { return localRotation; }

        /// \return Local scale.
        float GetLocalScale() const { return localScale; }
        
        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }
        
        /// \param localPosition Local position.
        /// \param center Point we're looking at.
        /// \param up Up vector.
        void LookAt( const Vec3& localPosition, const Vec3& center, const Vec3& up );

        /// \param amount Amount.
        void MoveRight( float amount );

        /// \param amount Amount.
        void MoveForward( float amount );

        /// \param amount Amount.
        void MoveUp( float amount );

        /// \param axis Axis to rotate about.
        /// \param angleDegrees Angle in degrees to be added to current rotation.
        void OffsetRotate( const Vec3& axis, float angleDegrees );
        
        /// \param localPos Local position.
        void SetLocalPosition( const Vec3& localPos );

        /// \param localRotation Local rotation.
        void SetLocalRotation( const Quaternion& localRotation );
        
        /// \param localScale Local scale.
        void SetLocalScale( float localScale );

        /// \param parent Parent transform or null if there is no parent.
        void SetParent( TransformComponent* parent );

        /// \return View matrix for VR that was set by SetVrView.
#if OCULUS_RIFT
        const Matrix44& GetVrView() const { return hmdView; }
#else
        const Matrix44& GetVrView() const { return Matrix44::identity; }
#endif
        /// \param view View matrix for VR viewing.
        void SetVrView( const Matrix44& view );

        /// \return Local transform matrix.
        const Matrix44& GetLocalMatrix();
        
        /// \return Textual representation of component.
        std::string GetSerialized() const;

        /// \return View direction (normalized)
        Vec3 GetViewDirection() const;

    private:
        friend class GameObject;
        friend class Scene;

        /// \return Component's type code. Must be unique for each component type.
        static int Type() { return 2; }
        
        /// \return Component handle that uniquely identifies the instance.
        static unsigned New();
        
        /// \return Component at index or null if index is invalid.
        static TransformComponent* Get( unsigned index );

        /// Updates matrices.
        static void UpdateLocalMatrices();

        void SolveLocalMatrix();

        void UpdateViewMatrix();
        
        Vec3 localPosition;
        Quaternion localRotation;
        float localScale = 1;
        Matrix44 localMatrix;
        TransformComponent* parent = nullptr;
#if OCULUS_RIFT
        Matrix44 hmdView; // For VR
#endif
        GameObject* gameObject = nullptr;
    };
}

#endif
