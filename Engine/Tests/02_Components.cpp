#include <iostream>
#include "CameraComponent.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "Mesh.hpp"
#include "MeshRendererComponent.hpp"
#include "RenderTexture.hpp"
#include "SpriteRendererComponent.hpp"
#include "System.hpp"
#include "TransformComponent.hpp"
#include "TextRendererComponent.hpp"
#include "Vec3.hpp"
#include "Window.hpp"

using namespace ae3d;

GameObject gGo;
RenderTexture gRenderTexture;

void TestManyInstances()
{
    const int instanceCount = 20;
    GameObject gos[ instanceCount ];

    for (int i = 0; i < instanceCount; ++i)
    {
        gos[ i ].AddComponent< CameraComponent >();
        gos[ i ].AddComponent< TransformComponent >();
        gos[ i ].AddComponent< MeshRendererComponent >();
        gos[ i ].AddComponent< SpriteRendererComponent >();
        gos[ i ].AddComponent< TextRendererComponent >();
    }
}

void TestCamera()
{
    gGo.AddComponent< CameraComponent >();
    gGo.GetComponent< CameraComponent >()->SetTargetTexture( &gRenderTexture );
    System::Assert( gGo.GetComponent< CameraComponent >()->GetTargetTexture() == &gRenderTexture, "camera render texture failed" );

    GameObject go2;
    go2.AddComponent< CameraComponent >();
    System::Assert( gGo.GetComponent< CameraComponent >() != go2.GetComponent< CameraComponent >(), "AddComponent produced identical references!" );
}

void TestAddition()
{
    GameObject go1;
    go1.AddComponent< CameraComponent >();
    go1.AddComponent< TransformComponent >();
    go1.AddComponent< MeshRendererComponent >();

    GameObject go2;
    go2.AddComponent< CameraComponent >();
    go2.AddComponent< TransformComponent >();
    go2.AddComponent< MeshRendererComponent >();

    System::Assert( go1.GetComponent< CameraComponent >() != go2.GetComponent< CameraComponent >(), "AddComponent produced identical references!" );
    System::Assert( go1.GetComponent< TransformComponent >() != go2.GetComponent< TransformComponent >(), "AddComponent produced identical references!" );
    System::Assert( go1.GetComponent< MeshRendererComponent >() != go2.GetComponent< MeshRendererComponent >(), "AddComponent produced identical references!" );
}

void TestTransform()
{
    const Vec3 pos{ 1, 2, 3 };
    gGo.AddComponent< TransformComponent >();
    gGo.GetComponent< TransformComponent >()->SetLocalPosition( pos );
    System::Assert( gGo.GetComponent< TransformComponent >()->GetLocalPosition().IsAlmost( pos ), "transform position failed" );
    
    TransformComponent copy = *gGo.GetComponent< TransformComponent >();
    System::Assert( copy.GetLocalPosition().x == gGo.GetComponent< TransformComponent >()->GetLocalPosition().x &&
                    copy.GetLocalPosition().y == gGo.GetComponent< TransformComponent >()->GetLocalPosition().y &&
                    copy.GetLocalPosition().z == gGo.GetComponent< TransformComponent >()->GetLocalPosition().z,
                    "Transform copy failed (position)!" );

    System::Assert( copy.GetLocalRotation().x == gGo.GetComponent< TransformComponent >()->GetLocalRotation().x &&
                    copy.GetLocalRotation().y == gGo.GetComponent< TransformComponent >()->GetLocalRotation().y &&
                    copy.GetLocalRotation().z == gGo.GetComponent< TransformComponent >()->GetLocalRotation().z &&
                    copy.GetLocalRotation().w == gGo.GetComponent< TransformComponent >()->GetLocalRotation().w,
                    "Transform copy failed (rotation)!" );
    
    System::Assert( copy.GetLocalScale() == gGo.GetComponent< TransformComponent >()->GetLocalScale(),
                   "transform copy failed (scale)!" );
}

void TestText()
{
    gGo.AddComponent< TextRendererComponent >();
    gGo.GetComponent< TextRendererComponent >()->SetText( "aether3d" );
    gGo.GetComponent< TextRendererComponent >()->SetText( "a" );
    gGo.GetComponent< TextRendererComponent >()->SetText( "" );
    gGo.GetComponent< TextRendererComponent >()->SetText( nullptr );
    
    TextRendererComponent copy = *gGo.GetComponent< TextRendererComponent >();
}

void TestSprite()
{
    gGo.AddComponent< SpriteRendererComponent >();
    gGo.GetComponent< SpriteRendererComponent >()->SetTexture( nullptr, { 0, 0, 0 }, { 10, 10, 1 }, { 1, 0, 0, 1 } );
    
    SpriteRendererComponent copy = *gGo.GetComponent< SpriteRendererComponent >();
}

void TestMesh()
{
    Mesh mesh;
    
    Mesh::LoadResult loadResult = mesh.Load( FileSystem::FileContents( "notfound" ) );
    System::Assert( loadResult == Mesh::LoadResult::FileNotFound, "Mesh load result FileNotFound failed!" );

    loadResult = mesh.Load( FileSystem::FileContents( "Makefile.math" ) );
    System::Assert( loadResult == Mesh::LoadResult::Corrupted, "Mesh load result Corrupted failed!" );
    
    gGo.AddComponent< MeshRendererComponent >();
    gGo.GetComponent< MeshRendererComponent >()->SetMesh( nullptr );
    
    Mesh copy = mesh;
    System::Assert( copy.GetAABBMin().x == mesh.GetAABBMin().x &&
                    copy.GetAABBMin().y == mesh.GetAABBMin().y &&
                    copy.GetAABBMin().z == mesh.GetAABBMin().z,
                    "Mesh copy failed!" );
}

int main()
{
    Window::Create( 512, 512, WindowCreateFlags::Empty );
    System::LoadBuiltinAssets();
    
    TestCamera();
    TestTransform();
    TestText();
    TestSprite();
    TestManyInstances();
    TestMesh();
    TestAddition();
}

    
