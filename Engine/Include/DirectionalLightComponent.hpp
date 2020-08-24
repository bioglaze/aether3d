#pragma once

#include "RenderTexture.hpp"
#include "Vec3.hpp"

namespace ae3d
{
    /// Directional light illuminates the Scene from a given direction. Ideal for sunlight.
    class DirectionalLightComponent
    {
    public:
		DirectionalLightComponent() noexcept : shadowMap() {}

        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }

        /// \param enabled True if the component should be rendered, false otherwise.
        void SetEnabled( bool enabled ) { isEnabled = enabled; }

        /// \return Color
        const Vec3& GetColor() const { return color; }

        /// \return Color
        Vec3& GetColor() { return color; }

        /// \param aColor Color in range 0-1.
        void SetColor( const Vec3& aColor ) { color = aColor; }

        /// \return True, if the light casts a shadow.
        bool CastsShadow() const { return castsShadow; }
        
        /// \return True, if enabled
        bool IsEnabled() const { return isEnabled; }
        
        /// \param enable If true, the light will cast a shadow.
        /// \param shadowMapSize Shadow map size in pixels. If it's invalid, it falls back to 512.
        void SetCastShadow( bool enable, int shadowMapSize );

        /// \return Shadow map
        RenderTexture* GetShadowMap() { return &shadowMap; }
        
    private:
        friend class GameObject;
        friend class Scene;

        /// \return Component's type code. Must be unique for each component type.
        static int Type() { return 6; }

        /// \return Component handle that uniquely identifies the instance.
        static unsigned New();

        /// \return Component at index or null if index is invalid.
        static DirectionalLightComponent* Get( unsigned index );

        RenderTexture shadowMap;
        GameObject* gameObject = nullptr;
        bool castsShadow = false;
        bool isEnabled = true;
        Vec3 color{ 1, 1, 1 };
    };
}
