#include "SceneView.hpp"
#include "CameraComponent.hpp"
#include "GameObject.hpp"

using namespace ae3d;

void SceneView::Init()
{
    camera.AddComponent< CameraComponent >();
}

void SceneView::Render()
{
    scene.Render();
    //DrawNuklear( &ctx, &cmds, width, height );
    scene.EndFrame();
}