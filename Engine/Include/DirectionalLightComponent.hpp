#ifndef DIRECTIONAL_LIGHT_HPP
#define DIRECTIONAL_LIGHT_HPP

#include <string>
#include "RenderTexture.hpp"
#include "Vec3.hpp"

namespace ae3d
{
    /// Directional light illuminates the Scene from a given direction. Ideal for sunlight.
    class DirectionalLightComponent
    {
    public:
        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }

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

        /// \return Serialized data.
        std::string GetSerialized() const;
        
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
        Vec3 color{ 1, 1, 1 };
    };
}
#endif
