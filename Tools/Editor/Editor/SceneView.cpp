#include "SceneView.hpp"
#include "CameraComponent.hpp"
#include "GameObject.hpp"
#include "MeshRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "Vec3.hpp"

using namespace ae3d;

void ScreenPointToRay( int screenX, int screenY, float screenWidth, float screenHeight, GameObject& camera, Vec3& outRayOrigin, Vec3& outRayTarget )
{
    const float aspect = screenHeight / screenWidth;
    const float halfWidth = screenWidth * 0.5f;
    const float halfHeight = screenHeight * 0.5f;
    const float fov = camera.GetComponent< CameraComponent >()->GetFovDegrees() * (3.1415926535f / 180.0f);

    // Normalizes screen coordinates and scales them to the FOV.
    const float dx = std::tan( fov * 0.5f ) * (screenX / halfWidth - 1.0f) / aspect;
    const float dy = std::tan( fov * 0.5f ) * (screenY / halfHeight - 1.0f);

    Matrix44 view;
    camera.GetComponent< TransformComponent >()->GetLocalRotation().GetMatrix( view );
    Matrix44 translation;
    translation.Translate( -camera.GetComponent< TransformComponent >()->GetLocalPosition() );
    Matrix44::Multiply( translation, view, view );

    Matrix44 invView;
    Matrix44::Invert( view, invView );

    const float farp = camera.GetComponent< CameraComponent >()->GetFar();

    outRayOrigin = camera.GetComponent< TransformComponent >()->GetLocalPosition();
    outRayTarget = -Vec3( -dx * farp, dy * farp, farp );

    Matrix44::TransformPoint( outRayTarget, invView, &outRayTarget );
}

template< typename T > class Array
{
  public:
    ~Array()
    {
        delete[] elements;
    }

    T& operator[]( std::size_t index ) const
    {
        return elements[ index ];
    }
    
    std::size_t GetLength() const
    {
        return elementCount;
    }

    void PushBack( const T& item )
    {
        T* after = new T[ elementCount + 1 ];

        for (std::size_t index = 0; index < elementCount; ++index)
        {
            after[ index ] = elements[ index ];
        }

        after[ elementCount ] = item;
        delete[] elements;
        elements = after;
        elementCount = elementCount + 1;
    }
    
    void Allocate( std::size_t size )
    {
        elements = new T[ size ];
        elementCount = size;
    }
    
  private:
    T* elements = nullptr;
    std::size_t elementCount = 0;
};

struct CollisionInfo
{
    int x;
};

Array< CollisionInfo > GetColliders( GameObject& camera, int screenX, int screenY, int width, int height, float maxDistance )
{
    Vec3 rayOrigin, rayTarget;
    ScreenPointToRay( screenX, screenY, width, height, camera, rayOrigin, rayTarget );
    
    Array< CollisionInfo > outColliders;
    outColliders.Allocate( 10 );
    outColliders[ 0 ].x = 1;
    
    return outColliders;
}

void SceneView::Init( int width, int height )
{
    camera.AddComponent< CameraComponent >();
    camera.GetComponent< CameraComponent >()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Perspective );
    camera.GetComponent< CameraComponent >()->SetProjection( 45, (float)width / (float)height, 1, 400 );
    camera.GetComponent< CameraComponent >()->SetClearColor( Vec3( 0.1f, 0.1f, 0.1f ) );
    camera.GetComponent< CameraComponent >()->SetClearFlag( ae3d::CameraComponent::ClearFlag::DepthAndColor );
    camera.AddComponent< TransformComponent >();
    camera.GetComponent< TransformComponent >()->LookAt( { 0, 0, 0 }, { 0, 0, 100 }, { 0, 1, 0 } );

    scene.Add( &camera );

    unlitShader.Load( FileSystem::FileContents( "unlit.vsh" ), FileSystem::FileContents( "unlit.fsh" ),
                      "unlit_vertex", "unlit_fragment",
                      FileSystem::FileContents( "unlit_vert.obj" ), FileSystem::FileContents( "unlit_frag.obj" ),
                      FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );

    transformGizmo.Init( &unlitShader );
    
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

    // Test code
    scene.Add( &transformGizmo.go );
    transformGizmo.xAxisMaterial.SetTexture( "textureMap", &gliderTex );
    transformGizmo.yAxisMaterial.SetTexture( "textureMap", &gliderTex );
    transformGizmo.zAxisMaterial.SetTexture( "textureMap", &gliderTex );
}

void SceneView::MoveCamera( const ae3d::Vec3& moveDir )
{
    camera.GetComponent< TransformComponent >()->MoveUp( moveDir.y );
    camera.GetComponent< TransformComponent >()->MoveForward( moveDir.z );
    camera.GetComponent< TransformComponent >()->MoveRight( moveDir.x );
}

void SceneView::BeginRender()
{
    scene.Render();
}

void SceneView::EndRender()
{
    scene.EndFrame();
}

void SceneView::RotateCamera( float xDegrees, float yDegrees )
{
    camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), xDegrees );
    camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 1, 0, 0 ), yDegrees );
}

void TransformGizmo::Init( ae3d::Shader* shader )
{
    translateMesh.Load( FileSystem::FileContents( "cursor_translate.ae3d" ) );
    
    xAxisMaterial.SetShader( shader );
    //xAxisMaterial.SetTexture( "textureMap", &translateTex );
    xAxisMaterial.SetVector( "tint", { 1, 0, 0, 1 } );
    xAxisMaterial.SetBackFaceCulling( true );
    xAxisMaterial.SetDepthFunction( ae3d::Material::DepthFunction::LessOrEqualWriteOn );
    float factor = -100;
    float units = 0;
    xAxisMaterial.SetDepthOffset( factor, units );
    yAxisMaterial.SetDepthOffset( factor, units );
    zAxisMaterial.SetDepthOffset( factor, units );
    
    yAxisMaterial.SetShader( shader );
    //yAxisMaterial.SetTexture( "textureMap", &translateTex );
    yAxisMaterial.SetVector( "tint", { 0, 1, 0, 1 } );
    yAxisMaterial.SetBackFaceCulling( true );
    yAxisMaterial.SetDepthFunction( ae3d::Material::DepthFunction::LessOrEqualWriteOn );
    
    zAxisMaterial.SetShader( shader );
    //zAxisMaterial.SetTexture( "textureMap", &translateTex );
    zAxisMaterial.SetVector( "tint", { 0, 0, 1, 1 } );
    zAxisMaterial.SetBackFaceCulling( true );
    zAxisMaterial.SetDepthFunction( ae3d::Material::DepthFunction::LessOrEqualWriteOn );
    
    go.AddComponent< MeshRendererComponent >();
    go.GetComponent< MeshRendererComponent >()->SetMesh( &translateMesh );
    go.GetComponent< MeshRendererComponent >()->SetMaterial( &xAxisMaterial, 1 );
    go.GetComponent< MeshRendererComponent >()->SetMaterial( &yAxisMaterial, 2 );
    go.GetComponent< MeshRendererComponent >()->SetMaterial( &zAxisMaterial, 0 );
    go.AddComponent< TransformComponent >();
    go.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 10, -50 } );
}
