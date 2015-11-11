#include <iostream>
#include <vector>
#include "CameraComponent.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "Mesh.hpp"
#include "MeshRendererComponent.hpp"
#include "RenderTexture.hpp"
#include "SpriteRendererComponent.hpp"
#include "System.hpp"
#include "Scene.hpp"
#include "TransformComponent.hpp"
#include "TextRendererComponent.hpp"
#include "Vec3.hpp"
#include "Window.hpp"

using namespace ae3d;

Scene gScene;
std::vector< GameObject > gGameObjects;

int main()
{
    Window::Create( 512, 512, WindowCreateFlags::Empty );
    System::LoadBuiltinAssets();
    
    Scene::DeserializeResult res = gScene.Deserialize( FileSystem::FileContents( "scene.scene" ), gGameObjects );
    System::Assert( res == Scene::DeserializeResult::Success, "Scene deserializing failed!" );
}


