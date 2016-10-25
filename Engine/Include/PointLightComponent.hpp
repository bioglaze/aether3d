#ifndef POINT_LIGHT_HPP
#define POINT_LIGHT_HPP

#include <string>
#include "RenderTexture.hpp"

namespace ae3d
{
    /// Point light illuminates the scene from a given position into all directions.
    class PointLightComponent
    {
    public:
        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }

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
        
        /// \return Serialized data.
        std::string GetSerialized() const;
        
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
        float radius = 10;
        GameObject* gameObject = nullptr;
        bool castsShadow = false;
    };
}
#endif
