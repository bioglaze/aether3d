#ifndef SPOT_LIGHT_HPP
#define SPOT_LIGHT_HPP

#include <string>
#include "RenderTexture.hpp"

namespace ae3d
{
    /// Spot light illuminates the scene from a given position with a viewing cone.
    class SpotLightComponent
    {
    public:
        /// \return True, if the light casts a shadow.
        bool CastsShadow() const { return castsShadow; }
        
        /// \param enable If true, the light will cast a shadow.
        /// \param shadowMapSize Shadow map size in pixels. If it's invalid, it falls back to 512.
        void SetCastShadow( bool enable, int shadowMapSize );
        
        /// \return Cone angle in degrees.
        float GetConeAngle() const { return coneAngle; }
        
        /// \param degrees Angle in degrees.
        void SetConeAngle( float degrees ) { coneAngle = degrees; }
        
        /// \return Serialized data.
        std::string GetSerialized() const;
        
    private:
        friend class GameObject;
        friend class Scene;
        
        /// \return Component's type code. Must be unique for each component type.
        static int Type() { return 7; }
        
        /// \return Component handle that uniquely identifies the instance.
        static unsigned New();
        
        /// \return Component at index or null if index is invalid.
        static SpotLightComponent* Get( unsigned index );
        
        RenderTexture shadowMap;
        float coneAngle = 45;
        bool castsShadow = false;
    };
}
#endif