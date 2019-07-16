#pragma once

#include "RenderTexture.hpp"

namespace ae3d
{
    /// Point light illuminates the scene from a given position into all directions.
    class PointLightComponent
    {
    public:
		PointLightComponent() noexcept : shadowMap() {}

        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }

        /// \return True, if enabled
        bool IsEnabled() const { return isEnabled; }
        
        /// \param enabled True if the component should be rendered, false otherwise.
        void SetEnabled( bool enabled ) { isEnabled = enabled; }

        /// \return Color
        const Vec3& GetColor() const { return color; }

        /// \param aColor Color in range 0-1.
        void SetColor( const Vec3& aColor ) { color = aColor; }

        /// \return True, if the light casts a shadow.
        bool CastsShadow() const { return castsShadow; }
        
        /// \param enable If true, the light will cast a shadow.
        /// \param shadowMapSize Shadow map size in pixels. If it's invalid, it falls back to 512.
        void SetCastShadow( bool enable, int shadowMapSize );

        /// \return Shadow map
        RenderTexture* GetShadowMap() { return &shadowMap; }

        /// \return radius.
        float GetRadius() const { return radius; }
        
        /// \param aRadius radius
        void SetRadius( float aRadius ) { radius = aRadius; }
        
    private:
        friend class GameObject;
        friend class Scene;
        
        /// \return Component's type code. Must be unique for each component type.
        static int Type() { return 8; }
        
        /// \return Component handle that uniquely identifies the instance.
        static unsigned New();
        
        /// \return Component at index or null if index is invalid.
        static PointLightComponent* Get( unsigned index );
        
        RenderTexture shadowMap;
        Vec3 color{ 1, 1, 1 };
        float radius = 10;
        GameObject* gameObject = nullptr;
        bool castsShadow = false;
        bool isEnabled = true;
        int cachedShadowMapSize = 0;
    };
}
