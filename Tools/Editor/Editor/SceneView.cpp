#include "SceneView.hpp"
#include "CameraComponent.hpp"
#include "GameObject.hpp"
#include "MeshRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "Vec3.hpp"

using namespace ae3d;

void SceneView::Init( int width, int height )
{
    camera.AddComponent< CameraComponent >();
    camera.GetComponent<CameraComponent>()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Perspective );
    camera.GetComponent<CameraComponent>()->SetProjection( 45, (float)width / (float)height, 1, 400 );
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0.1f, 0.1f, 0.1f ) );
    camera.GetComponent<CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::DepthAndColor );
    camera.AddComponent<TransformComponent>();
    camera.GetComponent<TransformComponent>()->LookAt( { 0, 0, 0 }, { 0, 0, 100 }, { 0, 1, 0 } );

    scene.Add( &camera );

    unlitShader.Load( FileSystem::FileContents( "unlit.vsh" ), FileSystem::FileContents( "unlit.fsh" ),
                      "unlit_vertex", "unlit_fragment",
                      FileSystem::FileContents( "unlit_vert.hlsl" ), FileSystem::FileContents( "unlit_frag.hlsl" ),
                      FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );

    // Test content
    
    gliderTex.Load( FileSystem::FileContents( "glider.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );

    material.SetShader( &unlitShader );
    material.SetTexture( "textureMap", &gliderTex );
    material.SetBackFaceCulling( true );

    cubeMesh.Load( FileSystem::FileContents( "textured_cube.ae3d" ) );
    
    cube.AddComponent< MeshRendererComponent >();
    cube.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    cube.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );
    cube.AddComponent< TransformComponent >();
    cube.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 0, -20 } );
    cube.SetName( "cube" );
    scene.Add( &cube );
}

void SceneView::Render()
{
    camera.GetComponent<TransformComponent>()->MoveUp( moveDir.y );
    camera.GetComponent<TransformComponent>()->MoveForward( moveDir.z );
    camera.GetComponent<TransformComponent>()->MoveRight( moveDir.x );

    scene.Render();
    //DrawNuklear( &ctx, &cmds, width, height );
    scene.EndFrame();
}

void SceneView::RotateCamera( float xDegrees, float yDegrees )
{
    camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), xDegrees );
    camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 1, 0, 0 ), yDegrees );
}
