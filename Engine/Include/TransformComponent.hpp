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
        
        /// \return Local transform matrix.
        const Matrix44& GetLocalMatrix();
        
        /// \return Textual representation of component.
        std::string GetSerialized() const;

    private:
        friend class GameObject;

        /// \return Component's type code. Must be unique for each component type.
        static int Type() { return 2; }
        
        /// \return Component handle that uniquely identifies the instance.
        static unsigned New();
        
        /// \return Component at index or null if index is invalid.
        static TransformComponent* Get( unsigned index );

        void SolveLocalMatrix();

        void UpdateViewMatrix();
        
        Vec3 localPosition;
        Quaternion localRotation;
        float localScale = 1;
        Matrix44 localMatrix;
        TransformComponent* parent = nullptr;
        bool isDirty = true;
    };
}

#endif
