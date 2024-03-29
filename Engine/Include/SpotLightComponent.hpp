#pragma once

#include "RenderTexture.hpp"
#include "Vec3.hpp"

namespace ae3d
{
    /// Spot light illuminates the scene from a given position with a viewing cone.
    class SpotLightComponent
    {
    public:
		SpotLightComponent() noexcept : shadowMap() {}

        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }

        /// \return True, if enabled
        bool IsEnabled() const { return isEnabled; }
        
        /// \param enabled True if the component should be rendered, false otherwise.
        void SetEnabled( bool enabled ) { isEnabled = enabled; }

        /// \return Color
        const Vec3& GetColor() const { return color; }

        /// \return Color
        Vec3& GetColor() { return color; }

        /// \return True, if the light casts a shadow.
        bool CastsShadow() const { return castsShadow; }
        
        /// \param enable If true, the light will cast a shadow.
        /// \param shadowMapSize Shadow map size in pixels. If it's invalid, it falls back to 512.
        void SetCastShadow( bool enable, int shadowMapSize );

        /// \param aRadius for light culler. Defaults to 2.
        void SetRadius( float aRadius ) { radius = aRadius; }
        
        /// \return Radius for light culler.
        float GetRadius() const { return radius; }

        /// \return Radius for light culler.
        float& GetRadius() { return radius; }

        /// \return Shadow map
        RenderTexture* GetShadowMap() { return &shadowMap; }

        /// \return Cone angle in degrees.
        float GetConeAngle() const { return coneAngle; }

        /// \return Cone angle in degrees.
        float& GetConeAngle() { return coneAngle; }

        /// \param degrees Angle in degrees.
        void SetConeAngle( float degrees );

        /// \param aColor Color in range 0-1.
        void SetColor( const Vec3& aColor ) { color = aColor; }
        
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
        GameObject* gameObject = nullptr;
        Vec3 color{ 1, 1, 1 };
        float coneAngle = 45;
        float radius = 2;
        bool castsShadow = false;
        bool isEnabled = true;
        int cachedShadowMapSize = 0;
    };
}
