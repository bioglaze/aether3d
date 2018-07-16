#include "SceneView.hpp"
#include "CameraComponent.hpp"
#include "GameObject.hpp"
#include "MeshRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "System.hpp"
#include "Vec3.hpp"
#include <vector>
#include <cmath>

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

struct CollisionInfo
{
    float meshDistance;
    GameObject* go;
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
    return std::abs( f - v ) < 0.0001f;
}

float IntersectRayAABB( const Vec3& origin, const Vec3& target, const Vec3& min, const Vec3& max )
{
    const Vec3 dir = (target - origin).Normalized();

    Vec3 dirfrac;
    dirfrac.x = 1.0f / dir.x;
    dirfrac.y = 1.0f / dir.y;
    dirfrac.z = 1.0f / dir.z;

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

float IntersectRayTriangles( const Vec3& origin, const Vec3& target, const std::vector< Vec3 >& vertices )
{
    for (std::size_t ve = 0; ve < vertices.size() / 3; ve += 3)
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

Array< CollisionInfo > GetColliders( GameObject& camera, int screenX, int screenY, int width, int height, float maxDistance, Array< GameObject >& gameObjects, CollisionTest collisionTest )
{
    Vec3 rayOrigin, rayTarget;
    ScreenPointToRay( screenX, screenY, width, height, camera, rayOrigin, rayTarget );
    
    Array< CollisionInfo > outColliders;

    // Collects meshes that collide with the ray.
    for (std::size_t i = 0; i < gameObjects.GetLength(); ++i)
    {
        GameObject* go = &gameObjects[ i ];
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
            collisionInfo.meshDistance = meshDistance;

            for (unsigned subMeshIndex = 0; subMeshIndex < meshRenderer->GetMesh()->GetSubMeshCount(); ++subMeshIndex)
            {
                Vec3 subMeshMin, subMeshMax;
                Matrix44::TransformPoint( meshRenderer->GetMesh()->GetSubMeshAABBMin( subMeshIndex ), meshLocalToWorld, &subMeshMin );
                Matrix44::TransformPoint( meshRenderer->GetMesh()->GetSubMeshAABBMax( subMeshIndex ), meshLocalToWorld, &subMeshMax );

                std::vector< Vec3 > triangles = meshRenderer->GetMesh()->GetSubMeshFlattenedTriangles( subMeshIndex );
                const float subMeshDistance = collisionTest == CollisionTest::AABB ? IntersectRayAABB( rayOrigin, rayTarget, subMeshMin, subMeshMax )
                                                                                   : IntersectRayTriangles( rayOrigin, rayTarget, triangles );
            }

            outColliders.PushBack( collisionInfo );
        }
    }

    //auto sortFunction = [](const CollisionInfo& info1, const CollisionInfo& info2) { return info1.meshDistance < info2.meshDistance; };
    //std::sort( std::begin( outColliders ), std::end( outColliders ), sortFunction );

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
    
    gameObjects.PushBack( GameObject() );
    gameObjects[ 0 ].AddComponent< MeshRendererComponent >();
    gameObjects[ 0 ].GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    gameObjects[ 0 ].GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );
    gameObjects[ 0 ].AddComponent< TransformComponent >();
    gameObjects[ 0 ].GetComponent< TransformComponent >()->SetLocalPosition( { 0, 0, -20 } );
    gameObjects[ 0 ].SetName( "cube" );
    scene.Add( &gameObjects[ 0 ] );

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

void SceneView::SelectObject( int screenX, int screenY, int width, int height )
{
    Array< CollisionInfo > ci = GetColliders( camera, screenX, screenY, width, height, 200, gameObjects, CollisionTest::Triangles );
    System::Print( "collider size: %d\n", ci.GetLength() );
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
