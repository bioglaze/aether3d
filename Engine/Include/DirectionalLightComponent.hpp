#ifndef DIRECTIONAL_LIGHT_HPP
#define DIRECTIONAL_LIGHT_HPP

#include "RenderTexture.hpp"

namespace ae3d
{
    /// Directional light illuminates the scene from a given direction. Ideal for sunlight.
    class DirectionalLightComponent
    {
    public:
        /// \param enable If true, the light will cast a shadow.
        /// \param shadowMapSize Shadow map size in pixels.
        void SetCastShadow( bool enable, int shadowMapSize );

    private:
        friend class GameObject;

        /// \return Component's type code. Must be unique for each component type.
        static int Type() { return 6; }

        /// \return Component handle that uniquely identifies the instance.
        static unsigned New();

        /// \return Component at index or null if index is invalid.
        static DirectionalLightComponent* Get( unsigned index );

        RenderTexture shadowMap;
        bool castsShadow = false;
    };
}
#endif
