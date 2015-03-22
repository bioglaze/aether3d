#ifndef SCENE_H
#define SCENE_H

#include "GameObject.hpp"

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
    };
}
#endif
