#pragma once

#include "Quaternion.hpp"

namespace ae3d
{
    // Not used yet, does nothing!
    class DecalRendererComponent
    {
    public:
        /// Constructor.
        DecalRendererComponent();

        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }

        /// \return True, if the component is enabled.
        bool IsEnabled() const { return isEnabled; }
        
        /// \param enabled True if the component should be rendered, false otherwise.
        void SetEnabled( bool enabled ) { isEnabled = enabled; }
        
        /// \return Texture.
        class Texture2D* GetTexture() const { return texture; }
        
        /// \param theTexture Texture that's projected to the decal's area.
        void SetTexture( Texture2D* theTexture ) { texture = theTexture; }
        
        ///  \return Orientation
        Quaternion GetOrientation() const { return orientation; }

        ///  \param q Orientation
        void SetOrientation( struct Quaternion& q ) { orientation = q; }

    private:
        friend class GameObject;
        friend class Scene;
        
        /* \return Component's type code. Must be unique for each component type. */
        static int Type() { return 12; }
        
        /* \return Component handle that uniquely identifies the instance. */
        static unsigned New();
        
        /* \return Component at index or null if index is invalid. */
        static DecalRendererComponent* Get( unsigned index );
                
        GameObject* gameObject = nullptr;
        Texture2D* texture = nullptr;
        Quaternion orientation;
        bool isEnabled = true;
    };
}
