#include "SceneView.hpp"
#include "CameraComponent.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "MeshRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "System.hpp"
#include "Vec3.hpp"
#include <math.h>
#include <string>
#include <vector>

using namespace ae3d;

void ScreenPointToRay( int screenX, int screenY, float screenWidth, float screenHeight, GameObject& camera, Vec3& outRayOrigin, Vec3& outRayTarget )
{
    const float aspect = screenHeight / screenWidth;
    const float halfWidth = screenWidth * 0.5f;
    const float halfHeight = screenHeight * 0.5f;
    const float fov = camera.GetComponent< CameraComponent >()->GetFovDegrees() * (3.1415926535f / 180.0f);

    // Normalizes screen coordinates and scales them to the FOV.
    const float dx = tan( fov * 0.5f ) * (screenX / halfWidth - 1.0f) / aspect;
    const float dy = tan( fov * 0.5f ) * (screenY / halfHeight - 1.0f);

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

struct CollisionInfo
{
    GameObject* go;
    float meshDistance;
    int subMeshIndex;
};

enum class CollisionTest
{
    AABB,
    Triangles
};

float Max2( float a, float b )
{
    return a < b ? b : a;
}

float Min2( float a, float b )
{
    return a < b ? a : b;
}

bool AlmostEquals( float f, float v )
{
    return abs( f - v ) < 0.0001f;
}

float IntersectRayAABB( const Vec3& origin, const Vec3& target, const Vec3& min, const Vec3& max )
{
    const Vec3 dir = (target - origin).Normalized();
    const Vec3 dirfrac{ 1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z };

    const float t1 = (min.x - origin.x) * dirfrac.x;
    const float t2 = (max.x - origin.x) * dirfrac.x;
    const float t3 = (min.y - origin.y) * dirfrac.y;
    const float t4 = (max.y - origin.y) * dirfrac.y;
    const float t5 = (min.z - origin.z) * dirfrac.z;
    const float t6 = (max.z - origin.z) * dirfrac.z;

    const float tmin = Max2( Max2( Min2(t1, t2), Min2(t3, t4)), Min2(t5, t6) );
    const float tmax = Min2( Min2( Max2(t1, t2), Max2(t3, t4)), Max2(t5, t6) );

    // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
    if (tmax < 0)
    {
        return -1.0f;
    }

    // if tmin > tmax, ray doesn't intersect AABB
    if (tmin > tmax)
    {
        return -1.0f;
    }

    return tmin;
}

float IntersectRayTriangles( const Vec3& origin, const Vec3& target, const Vec3* vertices, int vertexCount )
{
    for (int ve = 0; ve < vertexCount / 3; ve += 3)
    {
        const Vec3& v0 = vertices[ ve * 3 + 0 ];
        const Vec3& v1 = vertices[ ve * 3 + 1 ];
        const Vec3& v2 = vertices[ ve * 3 + 2 ];

        const Vec3 e1 = v1 - v0;
        const Vec3 e2 = v2 - v0;

        const Vec3 d = (target - origin).Normalized();

        const Vec3 h = Vec3::Cross( d, e2 );
        const float a = Vec3::Dot( e1, h );

        if (AlmostEquals( a, 0 ))
        {
            continue;
        }

        const float f = 1.0f / a;
        const Vec3 s = origin - v0;
        const float u = f * Vec3::Dot( s, h );

        if (u < 0 || u > 1)
        {
            continue;
        }

        const Vec3 q = Vec3::Cross( s, e1 );
        const float v = f * Vec3::Dot( d, q );

        if (v < 0 || u + v > 1)
        {
            continue;
        }

        const float t = f * Vec3::Dot( e2, q );

        if (t > 0.0001f)
        {
            return t;
        }
    }

    return -1;
}

void GetColliders( GameObject& camera, int screenX, int screenY, int width, int height, float maxDistance, Array< GameObject* >& gameObjects, CollisionTest collisionTest, Array< CollisionInfo >& outColliders )
{
    Vec3 rayOrigin, rayTarget;
    ScreenPointToRay( screenX, screenY, width, height, camera, rayOrigin, rayTarget );
    
    // Collects meshes that collide with the ray.
    for (int i = 0; i < gameObjects.count; ++i)
    {
        GameObject* go = gameObjects[ i ];
        auto meshRenderer = go->GetComponent< MeshRendererComponent >();

        if (!meshRenderer || !meshRenderer->GetMesh())
        {
            continue;
        }

        auto meshLocalToWorld = go->GetComponent< TransformComponent >() ? go->GetComponent< TransformComponent >()->GetLocalMatrix() : Matrix44::identity;
        Vec3 oMin, oMax;
        Matrix44::TransformPoint( meshRenderer->GetMesh()->GetAABBMin(), meshLocalToWorld, &oMin );
        Matrix44::TransformPoint( meshRenderer->GetMesh()->GetAABBMax(), meshLocalToWorld, &oMax );

        const float meshDistance = IntersectRayAABB( rayOrigin, rayTarget, oMin, oMax );

        if (0 < meshDistance && meshDistance < maxDistance)
        {
            CollisionInfo collisionInfo;
            collisionInfo.go = go;
            collisionInfo.meshDistance = 99999;
            collisionInfo.subMeshIndex = 0;
            
            for (unsigned subMeshIndex = 0; subMeshIndex < meshRenderer->GetMesh()->GetSubMeshCount(); ++subMeshIndex)
            {
                Vec3 subMeshMin, subMeshMax;
                Matrix44::TransformPoint( meshRenderer->GetMesh()->GetSubMeshAABBMin( subMeshIndex ), meshLocalToWorld, &subMeshMin );
                Matrix44::TransformPoint( meshRenderer->GetMesh()->GetSubMeshAABBMax( subMeshIndex ), meshLocalToWorld, &subMeshMax );

                std::vector< Vec3 > triangles = meshRenderer->GetMesh()->GetSubMeshFlattenedTriangles( subMeshIndex );
                const float subMeshDistance = collisionTest == CollisionTest::AABB ? IntersectRayAABB( rayOrigin, rayTarget, subMeshMin, subMeshMax )
                                                                                   : IntersectRayTriangles( rayOrigin, rayTarget, triangles.data(), (int)triangles.size() );

                System::Print("distance to submesh %d: %f. rayOrigin: %.2f, %.2f, %.2f, rayTarget: %.2f, %.2f, %.2f\n", subMeshIndex, subMeshDistance, rayOrigin.x, rayOrigin.y, rayOrigin.z, rayTarget.x, rayTarget.y, rayTarget.z);
                if (0 < subMeshDistance && subMeshDistance < collisionInfo.meshDistance)
                {
                    collisionInfo.subMeshIndex = subMeshIndex;
                    collisionInfo.meshDistance = subMeshDistance;
                }
            }

            outColliders.Add( collisionInfo );
        }
    }

    //auto sortFunction = [](const CollisionInfo& info1, const CollisionInfo& info2) { return info1.meshDistance < info2.meshDistance; };
    //std::sort( std::begin( outColliders ), std::end( outColliders ), sortFunction );
}

void SceneView::Init( int width, int height )
{
    camera.AddComponent< CameraComponent >();
    camera.GetComponent< CameraComponent >()->SetProjectionType( CameraComponent::ProjectionType::Perspective );
    camera.GetComponent< CameraComponent >()->SetProjection( 45, (float)width / (float)height, 1, 400 );
    camera.GetComponent< CameraComponent >()->SetClearColor( Vec3( 0.1f, 0.1f, 0.1f ) );
    camera.GetComponent< CameraComponent >()->SetClearFlag( CameraComponent::ClearFlag::DepthAndColor );
    camera.AddComponent< TransformComponent >();
    camera.GetComponent< TransformComponent >()->LookAt( { 0, 0, 0 }, { 0, 0, 100 }, { 0, 1, 0 } );

    scene.Add( &camera );

    unlitShader.Load( "unlit_vertex", "unlit_fragment",
                      FileSystem::FileContents( "unlit_vert.obj" ), FileSystem::FileContents( "unlit_frag.obj" ),
                      FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );

    gameObjects.Add( new GameObject() );
    transformGizmo.Init( &unlitShader, *gameObjects[ 0 ] );

    // Test content
    
    gliderTex.Load( FileSystem::FileContents( "glider.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );

    material.SetShader( &unlitShader );
    material.SetTexture( &gliderTex, 0 );
    material.SetBackFaceCulling( true );

    cubeMesh.Load( FileSystem::FileContents( "textured_cube.ae3d" ) );
    
    gameObjects.Add( new GameObject() );
    gameObjects[ 1 ]->AddComponent< MeshRendererComponent >();
    gameObjects[ 1 ]->GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    gameObjects[ 1 ]->GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );
    gameObjects[ 1 ]->AddComponent< TransformComponent >();
    gameObjects[ 1 ]->GetComponent< TransformComponent >()->SetLocalPosition( { 0, 0, -20 } );
    gameObjects[ 1 ]->SetName( "cube" );
    scene.Add( gameObjects[ 1 ] );

    // Test code
    transformGizmo.xAxisMaterial.SetTexture( &gliderTex, 0 );
    transformGizmo.yAxisMaterial.SetTexture( &gliderTex, 0 );
    transformGizmo.zAxisMaterial.SetTexture( &gliderTex, 0 );
}

void SceneView::MoveCamera( const Vec3& moveDir )
{
    camera.GetComponent< TransformComponent >()->MoveUp( moveDir.y );
    camera.GetComponent< TransformComponent >()->MoveForward( moveDir.z );
    camera.GetComponent< TransformComponent >()->MoveRight( moveDir.x );
}

void SceneView::MoveSelection( const Vec3& moveDir )
{
    if (selectedGameObjects.count > 0 && selectedGameObjects[ 0 ]->GetComponent< TransformComponent >() != nullptr)
    {
        selectedGameObjects[ 0 ]->GetComponent< TransformComponent >()->SetLocalPosition( selectedGameObjects[ 0 ]->GetComponent<TransformComponent>()->GetLocalPosition() + moveDir );
    }
}

void SceneView::BeginRender()
{
    scene.Render();
}

void SceneView::EndRender()
{
    scene.EndFrame();
}

void SceneView::LoadScene( const ae3d::FileSystem::FileContentsData& contents )
{
    std::vector< ae3d::GameObject > gos;
    std::map< std::string, class Texture2D* > texture2Ds;
    std::map< std::string, class Material* > materials;
    std::vector< class Mesh* > meshes;
    Scene::DeserializeResult result = scene.Deserialize( contents, gos, texture2Ds, materials, meshes );

    if (result == Scene::DeserializeResult::ParseError)
    {
        System::Print( "Could not parse scene file!\n" );
        return;
    }
                    
    /*gameObjects.clear();
                    
      for (auto& go : gos)
      {
      gameObjects.push_back( std::make_shared< ae3d::GameObject >() );
      *gameObjects.back() = go;
      scene.Add( gameObjects.back().get() );
      }*/
}

GameObject* SceneView::SelectObject( int screenX, int screenY, int width, int height )
{
    Array< CollisionInfo > ci;
    GetColliders( camera, screenX, screenY, width, height, 200, gameObjects, CollisionTest::Triangles, ci );

    if (ci.count > 0)
    {
        scene.Add( gameObjects[ 0 ] );
        gameObjects[ 0 ]->GetComponent< TransformComponent >()->SetLocalPosition( ci[ 0 ].go->GetComponent<TransformComponent>()->GetLocalPosition() );
        System::Print( "collided with submesh: %d\n", ci[ 0 ].subMeshIndex );

        selectedGameObjects.Add( ci[ 0 ].go );
        
        return ci[ 0 ].go;
    }

    selectedGameObjects.Allocate( 0 );
    scene.Remove( gameObjects[ 0 ] );
    return nullptr;
}

void SceneView::RotateCamera( float xDegrees, float yDegrees )
{
    camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), xDegrees );
    camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 1, 0, 0 ), yDegrees );
}

void TransformGizmo::Init( Shader* shader, GameObject& go )
{
    translateMesh.Load( FileSystem::FileContents( "cursor_translate.ae3d" ) );
    
    xAxisMaterial.SetShader( shader );
    //xAxisMaterial.SetTexture( &translateTex, 0 );
    xAxisMaterial.SetVector( "tint", { 1, 0, 0, 1 } );
    xAxisMaterial.SetBackFaceCulling( true );
    xAxisMaterial.SetDepthFunction( Material::DepthFunction::LessOrEqualWriteOn );
    float factor = -100;
    float units = 0;
    xAxisMaterial.SetDepthOffset( factor, units );
    yAxisMaterial.SetDepthOffset( factor, units );
    zAxisMaterial.SetDepthOffset( factor, units );
    
    yAxisMaterial.SetShader( shader );
    //yAxisMaterial.SetTexture( &translateTex, 0 );
    yAxisMaterial.SetVector( "tint", { 0, 1, 0, 1 } );
    yAxisMaterial.SetBackFaceCulling( true );
    yAxisMaterial.SetDepthFunction( Material::DepthFunction::LessOrEqualWriteOn );
    
    zAxisMaterial.SetShader( shader );
    //zAxisMaterial.SetTexture( &translateTex, 0 );
    zAxisMaterial.SetVector( "tint", { 0, 0, 1, 1 } );
    zAxisMaterial.SetBackFaceCulling( true );
    zAxisMaterial.SetDepthFunction( Material::DepthFunction::LessOrEqualWriteOn );

    go.AddComponent< MeshRendererComponent >();
    go.GetComponent< MeshRendererComponent >()->SetMesh( &translateMesh );
    go.GetComponent< MeshRendererComponent >()->SetMaterial( &xAxisMaterial, 1 );
    go.GetComponent< MeshRendererComponent >()->SetMaterial( &yAxisMaterial, 2 );
    go.GetComponent< MeshRendererComponent >()->SetMaterial( &zAxisMaterial, 0 );
    go.AddComponent< TransformComponent >();
    go.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 10, -50 } );
}

