#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

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

        /// \param localPos Local position.
        void SetLocalPosition( const Vec3& localPos );
        
        /* \return Local transform matrix. */
        const Matrix44& GetLocalMatrix() const { return localMatrix; }
        
    private:
        friend class GameObject;

        // \return Component's type code. Must be unique for each component type.
        static int Type() { return 2; }
        
        // \return Component handle that uniquely identifies the instance.
        static int New();
        
        // \return Component at index or null if index is invalid.
        static TransformComponent* Get( int index );

        Vec3 localPosition;
        Quaternion localRotation;
        int padding = 0;
        Matrix44 localMatrix;
    };
}

#endif
