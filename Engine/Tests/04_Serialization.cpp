#include <iostream>
#include <vector>
#include "Array.hpp"
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

int main()
{
    Window::Create( 512, 512, WindowCreateFlags::Empty );
    System::LoadBuiltinAssets();
    
    std::vector< GameObject > gameObjects;
    std::map< std::string, Material* > materialNameToMaterial;
    std::map< std::string, Texture2D* > textureNameToTexture;
    Array< Mesh* > meshes;
    Scene scene;
    
    auto res = scene.Deserialize( FileSystem::FileContents( "serialization_test.scene" ), gameObjects, textureNameToTexture,
                                 materialNameToMaterial, meshes );

    if (res != Scene::DeserializeResult::Success)
    {
        System::Print( "Scene serialization failed!\n" );
        return 1;
    }

    return 0;
}


