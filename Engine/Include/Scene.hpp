#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <string>

namespace ae3d
{
    class GameObject;
    class CameraComponent;
    class TextureCube;

    /// Contains game objects in a transform hierarchy.
    class Scene
    {
    public:
        /// Adds a game object into the scene if it does not exist there already.
        void Add( GameObject* gameObject );
        
        /// \param gameObject Game object to remove. Does nothing if it is null or doesn't exist in the scene.
        void Remove( GameObject* gameObject );
        
        /// Renders the scene.
        void Render();

        /// \param skyTexture Skybox texture. If this is the first time a valid skybox texture is provided, skybox geometry will also be generated.
        void SetSkybox( const TextureCube* skyTexture );
        
        /// \return Scene's contents in a textual format that can be saved into file etc.
        std::string GetSerialized() const;

    private:
        void RenderWithCamera( CameraComponent* camera );

        std::vector< GameObject* > gameObjects;
        unsigned nextFreeGameObject = 0;
        const TextureCube* skybox = nullptr;
        GameObject* mainCamera = nullptr;
    };
}
#endif
