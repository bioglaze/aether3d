#pragma once
#include "GameObject.hpp"
#include "Scene.hpp"

class SceneView
{
public:
    void Init();
    void Render();

private:
    ae3d::GameObject camera;
    ae3d::Scene scene;
};
