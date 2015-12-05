#include <iostream>
#include <string>
#include "CameraComponent.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "MeshRendererComponent.hpp"
#include "RenderTexture.hpp"
#include "SpriteRendererComponent.hpp"
#include "System.hpp"
#include "Scene.hpp"
#include "TransformComponent.hpp"
#include "TextRendererComponent.hpp"
#include "Texture2D.hpp"
#include "Vec3.hpp"
#include "Window.hpp"

using namespace ae3d;

int main()
{
    const int width = 512;
    const int height = 512;
    
    Window::Create( width, height, WindowCreateFlags::Empty );
    System::LoadBuiltinAssets();

    GameObject camera;
    camera.AddComponent<CameraComponent>();
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 1, 0, 0 ) );
    camera.GetComponent<CameraComponent>()->SetProjectionType( CameraComponent::ProjectionType::Perspective );
    camera.GetComponent<CameraComponent>()->SetProjection( 45, (float)width / (float)height, 1, 150 );
    camera.GetComponent<CameraComponent>()->SetClearFlag( CameraComponent::ClearFlag::DepthAndColor );
    camera.AddComponent<TransformComponent>();
    camera.GetComponent<TransformComponent>()->LookAt( { 0, 0, -80 }, { 0, 0, -100 }, { 0, 1, 0 } );

    Mesh cubeMesh;
    cubeMesh.Load( FileSystem::FileContents( "textured_cube.ae3d" ) );

    GameObject cube;
    cube.AddComponent< MeshRendererComponent >();
    cube.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    cube.AddComponent< TransformComponent >();
    cube.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 4, -100 } );

    Material material;
    // Testing a material without a shader.
    //material.SetShader( &shader );
    material.SetVector( "tint", { 1, 1, 1, 1 } );
    cube.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );

    GameObject cubeZero;
    cubeZero.AddComponent< MeshRendererComponent >();
    cubeZero.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    cubeZero.AddComponent< TransformComponent >();
    cubeZero.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 4, -100 } );
    cubeZero.GetComponent< TransformComponent >()->SetLocalScale( 0 );

    const char* atlas = R"(
                          <?xml version="1.0" encoding="UTF-8"?>
                          <!-- Created with TexturePacker http://www.codeandweb.com/texturepacker-->
                          <!-- $TexturePacker:SmartUpdate:aafe2d2d4a155a5a8dff4a1cd4b9ed14$ -->
                          <Imageset Name="atlas_cegui" Imagefile="atlas_cegui.png" NativeHorzRes="518" NativeVertRes="260" AutoScaled="false">
                          <Image Name="granite" XPos="2" YPos="2" Width="256" Height="256"/>
                          <Image Name="marble" XPos="260" YPos="2" Width="256" Height="256"/>
                          </Imageset>

                          )";
    std::string str( atlas );
    FileSystem::FileContentsData atlasContents;
    atlasContents.isLoaded = true;
    atlasContents.path = "atlas.xml";
    atlasContents.data.assign( std::begin( str ), std::end( str ) );
    
    Texture2D tex;
    tex.LoadFromAtlas( FileSystem::FileContents("atlas_cegui.png"), atlasContents, "granite", TextureWrap::Repeat, TextureFilter::Nearest, ColorSpace::RGB, 1 );

    Scene scene;
    scene.Add( &camera );
    scene.Add( &cube );
    scene.Add( &cubeZero );
    
    scene.Render();
    Window::SwapBuffers();
    System::Deinit();
}
