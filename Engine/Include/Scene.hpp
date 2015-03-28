#ifndef SCENE_H
#define SCENE_H

namespace ae3d
{
    class GameObject;

    class Scene
    {
    public:
        Scene();
        void Add( GameObject* gameObject );
        void Render();
        void Update();

    private:
        GameObject* gameObjects[10];
        GameObject* mainCamera = nullptr;
        int nextFreeGameObject = 0;
    };
}
#endif
