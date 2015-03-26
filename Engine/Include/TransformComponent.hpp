#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include "Vec3.hpp"
#include "Quaternion.hpp"

namespace ae3d
{
    class TransformComponent
    {
    public:
        // \return Component's type code. Must be unique for each component type.
        static int Type() { return 2; }
        
        // \return Component handle that uniquely identifies the instance.
        static int New();
        
        // \return Component at index or null if index is invalid.
        static TransformComponent* Get( int index );

    private:
        static TransformComponent transformComponents[100];
        Vec3 localPosition;
        Quaternion localRotation;
    };
}

#endif
