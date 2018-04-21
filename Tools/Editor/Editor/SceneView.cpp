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
                      "unlitVert", "unlitFrag",
                      FileSystem::FileContents( "unlit_vert.hlsl" ), FileSystem::FileContents( "unlit_frag.hlsl" ),
                      FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );

    // Test content
    
    gliderTex.Load( FileSystem::FileContents( "glider.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );

    material.SetShader( &unlitShader );
    material.SetTexture( "textureMap", &gliderTex );
    material.SetBackFaceCulling( true );

    cubeMesh.Load( FileSystem::FileContents( "cube.ae3d" ) );
    
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
    scene.Render();
    //DrawNuklear( &ctx, &cmds, width, height );
    scene.EndFrame();
}
