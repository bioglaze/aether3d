#ifndef SCENE_H
#define SCENE_H

namespace ae3d
{
    class GameObject;

    /// Contains game objects in a transform hierarchy.
    class Scene
    {
    public:
        /// Constructor.
        Scene();

        /// Adds a game object into the scene if it does not exist there already.
        void Add( GameObject* gameObject );

        /// Renders the scene.
        void Render();

        /// Updates simulation.
        void Update();

    private:
        GameObject* gameObjects[10];
        GameObject* mainCamera = nullptr;
        int nextFreeGameObject = 0;
    };
}
#endif
