// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "SceneView.hpp"
#include "Array.hpp"
#include "AudioSourceComponent.hpp"
#include "CameraComponent.hpp"
#include "ComputeShader.hpp"
#include "DirectionalLightComponent.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "MeshRendererComponent.hpp"
#include "PointLightComponent.hpp"
#include "Texture2D.hpp"
#include "TransformComponent.hpp"
#include "Scene.hpp"
#include "SpotLightComponent.hpp"
#include "Shader.hpp"
#include "System.hpp"
#include "Vec3.hpp"
#include <string>
#include <vector>
#include <math.h>
#include <stdio.h>

// *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)
struct pcg32_random_t
{
    uint64_t state;
    uint64_t inc;
};

uint32_t pcg32_random_r( pcg32_random_t* rng )
{
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc | 1);
    uint32_t xorshifted = uint32_t( ((oldstate >> 18u) ^ oldstate) >> 27u );
    int32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

pcg32_random_t rng;

int Random100()
{
    return pcg32_random_r( &rng ) % 100;
}

using namespace ae3d;

struct TransformGizmo
{
    void Init( Shader* shader, GameObject& go );
    
    Mesh mesh;
    
    Mesh translateMesh;
    Material xAxisMaterial;
    Material yAxisMaterial;
    Material zAxisMaterial;
    Texture2D redTex;
    Texture2D greenTex;
    Texture2D blueTex;

	int selectedMesh = 0;
};

struct SceneView
{
    Array< GameObject* > gameObjects;
    Array< GameObject* > selectedGameObjects;
    GameObject camera;
    Scene scene;
    Shader unlitShader;
    ComputeShader blurShader;
    ComputeShader downsampleAndThresholdShader;
    ComputeShader ssaoShader;
    TransformGizmo transformGizmo;
    Texture2D goTex;
    Texture2D audioTex;
    Texture2D lightTex;
    Texture2D cameraTex;
    Texture2D ssaoTex;
    Texture2D bloomTex;
    Texture2D blurTex;
    Texture2D blurTex2;
    Texture2D noiseTex;
    Matrix44 lineProjection;
    int lineHandle = 0;
    int selectedGOIndex = 0;
    Material highlightMaterial;
    RenderTexture cameraTarget;

    // TODO: Test content, remove when stuff works.
    Texture2D gliderTex;
    Material material;
    Mesh cubeMesh;
};

void ScreenPointToRay( int screenX, int screenY, float screenWidth, float screenHeight, GameObject& camera, Vec3& outRayOrigin, Vec3& outRayTarget )
{
    const float aspect = screenHeight / screenWidth;
    const float halfWidth = screenWidth * 0.5f;
    const float halfHeight = screenHeight * 0.5f;
    const float fov = camera.GetComponent< CameraComponent >()->GetFovDegrees() * (3.1415926535f / 180.0f);

    // Normalizes screen coordinates and scales them to the FOV.
    const float dx = tanf( fov * 0.5f ) * (screenX / halfWidth - 1.0f) / aspect;
    const float dy = tanf( fov * 0.5f ) * (screenY / halfHeight - 1.0f);
    
    Matrix44 view;
    camera.GetComponent< TransformComponent >()->GetLocalRotation().GetMatrix( view );
    Matrix44 translation;
    translation.SetTranslation( -camera.GetComponent< TransformComponent >()->GetLocalPosition() );
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
    GameObject* go = nullptr;
    float meshDistance = 0;
    int subMeshIndex = 0;
    int gameObjectIndex = 0;
};

enum class CollisionTest
{
    AABB,
    Triangles
};

CollisionTest colTest = CollisionTest::Triangles;

static float Min2( float a, float b )
{
    return a < b ? b : a;
}

static float Max2( float a, float b )
{
    return a < b ? b : a;
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

float IntersectRayTriangles( const Vec3& origin, const Vec3& target, const Vec3* vertices, unsigned vertexCount )
{
    for (unsigned ve = 0; ve < vertexCount / 3; ve += 3)
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

static void GetMinMax( const Vec3* aPoints, int count, Vec3& outMin, Vec3& outMax )
{
    outMin = aPoints[ 0 ];
    outMax = aPoints[ 0 ];

    for (int i = 1, s = count; i < s; ++i)
    {
        const Vec3& point = aPoints[ i ];

        if (point.x < outMin.x)
        {
            outMin.x = point.x;
        }

        if (point.y < outMin.y)
        {
            outMin.y = point.y;
        }

        if (point.z < outMin.z)
        {
            outMin.z = point.z;
        }

        if (point.x > outMax.x)
        {
            outMax.x = point.x;
        }

        if (point.y > outMax.y)
        {
            outMax.y = point.y;
        }

        if (point.z > outMax.z)
        {
            outMax.z = point.z;
        }
    }
}

static void GetCorners( const Vec3& min, const Vec3& max, Vec3 outCorners[ 8 ] )
{
    outCorners[ 0 ] = Vec3( min.x, min.y, min.z );
    outCorners[ 1 ] = Vec3( max.x, min.y, min.z );
    outCorners[ 2 ] = Vec3( min.x, max.y, min.z );
    outCorners[ 3 ] = Vec3( min.x, min.y, max.z );
    outCorners[ 4 ] = Vec3( max.x, max.y, min.z );
    outCorners[ 5 ] = Vec3( min.x, max.y, max.z );
    outCorners[ 6 ] = Vec3( max.x, max.y, max.z );
    outCorners[ 7 ] = Vec3( max.x, min.y, max.z );
}

void GetColliders( GameObject& camera, bool includeGizmo, int screenX, int screenY, int width, int height, float maxDistance, Array< GameObject* >& gameObjects, CollisionTest collisionTest, Array< CollisionInfo >& outColliders )
{
    Vec3 rayOrigin, rayTarget;
    ScreenPointToRay( screenX, screenY, (float)width, (float)height, camera, rayOrigin, rayTarget );
    
    // Collects meshes that collide with the ray.
    const unsigned startIndex = includeGizmo ? 0 : 1;
    
    for (unsigned i = startIndex; i < gameObjects.count; ++i)
    {
        GameObject* go = gameObjects[ i ];
        auto meshRenderer = go->GetComponent< MeshRendererComponent >();

        if (!meshRenderer || !meshRenderer->GetMesh())
        {
            continue;
        }

        auto meshLocalToWorld = go->GetComponent< TransformComponent >() ? go->GetComponent< TransformComponent >()->GetLocalMatrix() : Matrix44::identity;
        Vec3 oMin, oMax;
        Vec3 oAABB[ 8 ];
        GetCorners( meshRenderer->GetMesh()->GetAABBMin(), meshRenderer->GetMesh()->GetAABBMax(), oAABB );

        for (int v = 0; v < 8; ++v)
        {
            Matrix44::TransformPoint( oAABB[ v ], meshLocalToWorld, &oAABB[ v ] );
        }

        GetMinMax( oAABB, 8, oMin, oMax );

        const float meshDistance = IntersectRayAABB( rayOrigin, rayTarget, oMin, oMax );

        if (0 < meshDistance && meshDistance < maxDistance)
        {
            CollisionInfo collisionInfo;
            collisionInfo.go = go;
            collisionInfo.meshDistance = 99999;
            collisionInfo.subMeshIndex = -1;
            collisionInfo.gameObjectIndex = i;

            for (unsigned subMeshIndex = 0; subMeshIndex < meshRenderer->GetMesh()->GetSubMeshCount(); ++subMeshIndex)
            {
                Vec3 subMeshMin, subMeshMax;
                Vec3 mAABB[ 8 ];

                GetCorners( meshRenderer->GetMesh()->GetSubMeshAABBMin( subMeshIndex ), meshRenderer->GetMesh()->GetSubMeshAABBMax( subMeshIndex ), mAABB );

                for (int v = 0; v < 8; ++v)
                {
                    Matrix44::TransformPoint( mAABB[ v ], meshLocalToWorld, &mAABB[ v ] );
                }

                GetMinMax( mAABB, 8, subMeshMin, subMeshMax );

                Array< Vec3 > triangles;
                meshRenderer->GetMesh()->GetSubMeshFlattenedTriangles( subMeshIndex, triangles );
                
                for (unsigned v = 0; v < triangles.count; ++v)
                {
                    Matrix44::TransformPoint( triangles[ v ], meshLocalToWorld, &triangles[ v ] );
                }

                const float subMeshDistance = collisionTest == CollisionTest::AABB ? IntersectRayAABB( rayOrigin, rayTarget, subMeshMin, subMeshMax )
                                                                                   : IntersectRayTriangles( rayOrigin, rayTarget, triangles.elements, triangles.count );

                //System::Print("distance to submesh %d: %f. rayOrigin: %.2f, %.2f, %.2f, rayTarget: %.2f, %.2f, %.2f\n", subMeshIndex, subMeshDistance, rayOrigin.x, rayOrigin.y, rayOrigin.z, rayTarget.x, rayTarget.y, rayTarget.z);
                if (0 < subMeshDistance && subMeshDistance < collisionInfo.meshDistance)
                {
                    collisionInfo.subMeshIndex = subMeshIndex;
                    collisionInfo.meshDistance = subMeshDistance;
                }
            }

            if (collisionInfo.subMeshIndex != -1)
            {
                outColliders.Add( collisionInfo );
            }
        }
    }

    //auto sortFunction = [](const CollisionInfo& info1, const CollisionInfo& info2) { return info1.meshDistance < info2.meshDistance; };
    //std::sort( std::begin( outColliders ), std::end( outColliders ), sortFunction );
}

void svInit( SceneView** sv, int width, int height )
{
    *sv = new SceneView();
    
    (*sv)->cameraTarget.Create2D( width, height, ae3d::DataType::Float, TextureWrap::Clamp, TextureFilter::Linear, "cameraTarget", false );
    (*sv)->bloomTex.CreateUAV( width / 2, height / 2, "bloomTex", DataType::UByte );
    (*sv)->blurTex.CreateUAV( width / 2, height / 2, "blurTex", DataType::UByte );
    (*sv)->blurTex2.CreateUAV( width / 2, height / 2, "blur2Tex", DataType::UByte );
    (*sv)->ssaoTex.CreateUAV( width, height, "ssaoTex", DataType::Float );

    constexpr int noiseDim = 64;
    Vec4 noiseData[ noiseDim * noiseDim ];

    for (int i = 0; i < noiseDim * noiseDim; ++i)
    {
        Vec3 dir = Vec3( (Random100() / 100.0f), (Random100() / 100.0f), 0 ).Normalized();
        dir.x = abs( dir.x ) * 2 - 1;
        dir.y = abs( dir.y ) * 2 - 1;
        noiseData[ i ].x = dir.x;
        noiseData[ i ].y = dir.y;
        noiseData[ i ].z = 0;
        noiseData[ i ].w = 0;
    }

#if RENDERER_VULKAN    
    (*sv)->noiseTex.LoadFromData( noiseData, noiseDim, noiseDim, "noiseData", VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, DataType::Float );
#else
    (*sv)->noiseTex.LoadFromData( noiseData, noiseDim, noiseDim, "noiseData", DataType::Float );
#endif

    (*sv)->camera.AddComponent< CameraComponent >();
    (*sv)->camera.GetComponent< CameraComponent >()->SetProjectionType( CameraComponent::ProjectionType::Perspective );
    (*sv)->camera.GetComponent< CameraComponent >()->SetProjection( 45, (float)width / (float)height, 1, 400 );
    (*sv)->camera.GetComponent< CameraComponent >()->SetClearColor( Vec3( 0.1f, 0.1f, 0.1f ) );
    (*sv)->camera.GetComponent< CameraComponent >()->SetClearFlag( CameraComponent::ClearFlag::DepthAndColor );
    (*sv)->camera.GetComponent<CameraComponent>()->GetDepthNormalsTexture().Create2D( width, height, ae3d::DataType::Float, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Nearest, "depthnormals", false );
    (*sv)->camera.GetComponent<CameraComponent>()->SetTargetTexture( &(*sv)->cameraTarget );
    (*sv)->camera.AddComponent< TransformComponent >();
    (*sv)->camera.GetComponent< TransformComponent >()->LookAt( { 0, 2, 20 }, { 0, 0, 100 }, { 0, 1, 0 } );
    (*sv)->camera.SetName( "EditorCamera" );

    (*sv)->scene.Add( &(*sv)->camera );

    (*sv)->unlitShader.Load( "unlit_vertex", "unlit_fragment",
                      FileSystem::FileContents( "unlit_vert.obj" ), FileSystem::FileContents( "unlit_frag.obj" ),
                      FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );

    (*sv)->blurShader.Load( "blur", FileSystem::FileContents( "Blur.obj" ), FileSystem::FileContents( "Blur.spv" ) );
    (*sv)->downsampleAndThresholdShader.Load( "downsampleAndThreshold", FileSystem::FileContents( "Bloom.obj" ), FileSystem::FileContents( "Bloom.spv" ) );
    (*sv)->ssaoShader.Load( "ssao", FileSystem::FileContents( "ssao.obj" ), FileSystem::FileContents( "ssao.spv" ) );

    (*sv)->gameObjects.Add( new GameObject() );
    (*sv)->transformGizmo.redTex.Load( FileSystem::FileContents( "red.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::None, ColorSpace::SRGB, Anisotropy::k1 );
    (*sv)->transformGizmo.greenTex.Load( FileSystem::FileContents( "green.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::None, ColorSpace::SRGB, Anisotropy::k1 );
    (*sv)->transformGizmo.blueTex.Load( FileSystem::FileContents( "blue.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::None, ColorSpace::SRGB, Anisotropy::k1 );
    (*sv)->transformGizmo.Init( &(*sv)->unlitShader, *(*sv)->gameObjects[ 0 ] );

    // Test content
    
    (*sv)->gliderTex.Load( FileSystem::FileContents( "glider.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );
    (*sv)->cameraTex.Load( FileSystem::FileContents( "camera.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );
    (*sv)->lightTex.Load( FileSystem::FileContents( "light.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );
    (*sv)->goTex.Load( FileSystem::FileContents( "gameobject.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );
    (*sv)->audioTex.Load( FileSystem::FileContents( "audio_source.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );

    (*sv)->material.SetShader( &(*sv)->unlitShader );
    (*sv)->material.SetTexture( &(*sv)->gliderTex, 0 );
    (*sv)->material.SetBackFaceCulling( true );

    (*sv)->highlightMaterial.SetShader( &(*sv)->unlitShader );
    (*sv)->highlightMaterial.SetTexture( &(*sv)->lightTex, 0 );
    (*sv)->highlightMaterial.SetBackFaceCulling( true );

    (*sv)->cubeMesh.Load( FileSystem::FileContents( "textured_cube.ae3d" ) );
    
    (*sv)->gameObjects.Add( new GameObject() );
    (*sv)->gameObjects[ 1 ]->AddComponent< MeshRendererComponent >();
    (*sv)->gameObjects[ 1 ]->GetComponent< MeshRendererComponent >()->SetMesh( &(*sv)->cubeMesh );
    (*sv)->gameObjects[ 1 ]->GetComponent< MeshRendererComponent >()->SetMaterial( &(*sv)->material, 0 );
    (*sv)->gameObjects[ 1 ]->AddComponent< TransformComponent >();
    (*sv)->gameObjects[ 1 ]->GetComponent< TransformComponent >()->SetLocalPosition( { 0, 0, 0 } );
    (*sv)->gameObjects[ 1 ]->SetName( "cube" );
    (*sv)->scene.Add( (*sv)->gameObjects[ 1 ] );

    (*sv)->lineProjection.MakeProjection( 0, (float)width, (float)height, 0, 0, 1 );
    constexpr unsigned LineCount = 80;
    Vec3 lines[ LineCount ];
    
    for (unsigned i = 0; i < LineCount / 4; ++i)
    {
        lines[ i * 2 + 0 ] = Vec3( (float)i - 10, 0, -16 + 24 );
        lines[ i * 2 + 1 ] = Vec3( (float)i - 10, 0, -35 + 24 );
    }

    for (unsigned i = LineCount / 4; i < LineCount / 2; ++i)
    {
        lines[ i * 2 + 0 ] = Vec3( -5 + 24 - 10, 0, (float)i - 55 + 24 );
        lines[ i * 2 + 1 ] = Vec3( -24 + 24 - 10, 0, (float)i - 55 + 24 );
    }

    (*sv)->lineHandle = System::CreateLineBuffer( lines, LineCount, Vec3( 1, 1, 1 ) );
}

ae3d::Material* svGetMaterial( SceneView* sceneView )
{
    return &sceneView->material;
}

GameObject** svGetGameObjects( SceneView* sceneView, int& outCount )
{
    outCount = sceneView->gameObjects.count;
    return sceneView->gameObjects.elements;
}

void svMoveCamera( SceneView* sv, const Vec3& moveDir )
{
    sv->camera.GetComponent< TransformComponent >()->MoveUp( moveDir.y );
    sv->camera.GetComponent< TransformComponent >()->MoveForward( moveDir.z );
    sv->camera.GetComponent< TransformComponent >()->MoveRight( moveDir.x );
}

void svMoveSelection( SceneView* sv, const Vec3& moveDir )
{
    if (sv->selectedGameObjects.count > 0 && sv->selectedGameObjects[ 0 ]->GetComponent< TransformComponent >() != nullptr)
    {
        sv->selectedGameObjects[ 0 ]->GetComponent< TransformComponent >()->SetLocalPosition( sv->selectedGameObjects[ 0 ]->GetComponent<TransformComponent>()->GetLocalPosition() + moveDir );
    }
}

void svAddGameObject( SceneView* sv )
{
    sv->gameObjects.Add( new GameObject() );
    sv->gameObjects[ sv->gameObjects.count - 1 ]->SetName( "GameObject" );
    sv->gameObjects[ sv->gameObjects.count - 1 ]->AddComponent< TransformComponent >();
    sv->scene.Add( sv->gameObjects[ sv->gameObjects.count - 1 ] );
}

void svDuplicateGameObject( SceneView* sv )
{
    if (sv->selectedGameObjects.count > 0 && sv->selectedGameObjects[ 0 ])
    {
        sv->gameObjects.Add( new GameObject() );
        sv->gameObjects[ sv->gameObjects.count - 1 ]->SetName( "GameObject" );
        sv->gameObjects[ sv->gameObjects.count - 1 ]->AddComponent< TransformComponent >();
        sv->gameObjects[ sv->gameObjects.count - 1 ]->GetComponent< TransformComponent >()->SetLocalPosition( sv->selectedGameObjects[ 0 ]->GetComponent< TransformComponent >()->GetLocalPosition() );

        if (sv->selectedGameObjects[ 0 ]->GetComponent< MeshRendererComponent >())
        {
            sv->gameObjects[ sv->gameObjects.count - 1 ]->AddComponent< MeshRendererComponent >();
            sv->gameObjects[ sv->gameObjects.count - 1 ]->GetComponent< MeshRendererComponent >()->SetMesh( sv->selectedGameObjects[ 0 ]->GetComponent< MeshRendererComponent >()->GetMesh() );
            sv->gameObjects[ sv->gameObjects.count - 1 ]->GetComponent< MeshRendererComponent >()->SetMaterial( sv->selectedGameObjects[ 0 ]->GetComponent< MeshRendererComponent >()->GetMaterial( 0 ), 0 );
        }

        sv->scene.Add( sv->gameObjects[ sv->gameObjects.count - 1 ] );
    }
}

void svBeginRender( SceneView* sv )
{
    sv->scene.Render();
    System::Draw( &sv->cameraTarget, 0, 0, sv->cameraTarget.GetWidth(), sv->cameraTarget.GetHeight(), sv->cameraTarget.GetWidth(), sv->cameraTarget.GetHeight(), Vec4( 1, 1, 1, 1 ), System::BlendMode::Off );
    if (false)
    {
        int width = sv->ssaoTex.GetWidth();
        int height = sv->ssaoTex.GetHeight();

        sv->ssaoShader.SetProjectionMatrix( sv->camera.GetComponent<CameraComponent>()->GetProjection() );
        sv->ssaoShader.SetRenderTexture( 0, &sv->cameraTarget );
        sv->ssaoShader.SetRenderTexture( 1, &sv->camera.GetComponent<CameraComponent>()->GetDepthNormalsTexture() );
#if RENDERER_VULKAN
        sv->ssaoTex.SetLayout( TextureLayout::General );
        sv->ssaoShader.SetTexture2D( 2, &sv->noiseTex );
        sv->ssaoShader.SetTexture2D( 14, &sv->ssaoTex );
        sv->ssaoShader.Begin();
#else
        sv->ssaoShader.SetTexture2D( 3, &sv->noiseTex );
        sv->ssaoShader.SetTexture2D( 2, &sv->ssaoTex );
#endif
        sv->ssaoShader.Dispatch( width / 8, height / 8, 1, "SSAO" );
#if RENDERER_VULKAN
        sv->ssaoShader.End();
        sv->ssaoTex.SetLayout( TextureLayout::ShaderRead );
#endif
        System::Draw( &sv->ssaoTex, 0, 0, width, height, width, height, Vec4( 1, 1, 1, 1 ), System::BlendMode::Off );
    }
}

void svEndRender( SceneView* sv )
{
    sv->scene.EndFrame();
}

std::vector< ae3d::GameObject > gos;

void svLoadScene( SceneView* sv, const ae3d::FileSystem::FileContentsData& contents )
{
    gos.clear();
    std::map< std::string, class Texture2D* > texture2Ds;
    std::map< std::string, class Material* > materials;
    Array< class Mesh* > meshes;
    Scene::DeserializeResult result = sv->scene.Deserialize( contents, gos, texture2Ds, materials, meshes );

    if (result == Scene::DeserializeResult::ParseError)
    {
        System::Print( "Could not parse scene file!\n" );
        return;
    }
    
    for (auto& mat : materials)
    {
        mat.second->SetShader( &sv->unlitShader );
    }

    ae3d::GameObject* gizmo = sv->gameObjects[ 0 ];

    for (unsigned i = 0; i < sv->gameObjects.count; ++i)
    {
        sv->scene.Remove( sv->gameObjects[ i ] );
    }
    
    sv->gameObjects.Allocate( 1 );
    sv->gameObjects[ 0 ] = gizmo;

    for (auto& go : gos)
    {
        sv->gameObjects.Add( &go );
        sv->scene.Add( sv->gameObjects[ sv->gameObjects.count - 1 ] );
    }
}

void svSaveScene( SceneView* sv, char* path )
{
    sv->scene.Remove( &sv->camera );
    
    if (sv->gameObjects.count > 0 && sv->gameObjects[ 0 ] != nullptr)
    {
        sv->scene.Remove( sv->gameObjects[ 0 ] );
    }

    const std::string sceneStr = sv->scene.GetSerialized();
    FILE* f = fopen( path, "wb" );
    if (!f)
    {
        System::Print( "Could not open file for saving: %s\n", path );
        sv->scene.Add( &sv->camera );
        return;
    }
    
    fprintf( f, "%s\n", sceneStr.c_str() );
    fclose( f );

    sv->scene.Add( &sv->camera );
}

ae3d::GameObject* svSelectGameObjectIndex( SceneView* sv, int index )
{
    sv->selectedGameObjects.Allocate( 0 );
    sv->selectedGameObjects.Add( sv->gameObjects[ index ] );
    sv->selectedGOIndex = index;

    sv->scene.Add( sv->gameObjects[ 0 ] );
    sv->gameObjects[ 0 ]->GetComponent< TransformComponent >()->SetLocalPosition( sv->gameObjects[ index ]->GetComponent<TransformComponent>()->GetLocalPosition() );

    return sv->gameObjects[ index ];
}

GameObject* svSelectObject( SceneView* sv, int screenX, int screenY, int width, int height )
{
    ae3d::CameraComponent* camera = sv->camera.GetComponent<CameraComponent>();
    const unsigned texWidth = sv->lightTex.GetWidth();
    const unsigned texHeight = sv->lightTex.GetHeight();

    // Checks if the mouse hit a sprite and selects the object.
    for (unsigned goIndex = 1; goIndex < sv->gameObjects.count; ++goIndex)
    {
        TransformComponent* goTransform = sv->gameObjects[ goIndex ]->GetComponent< TransformComponent >();
        
        if (goTransform == nullptr)
        {
            continue;
        }

        Vec3 screenPoint = camera->GetScreenPoint( goTransform->GetLocalPosition(), (float)width, (float)height );
#if RENDERER_VULKAN
        float s = 1;
        screenPoint.y = height - screenPoint.y;
        //screenPoint.y += 10; // Tested on windows by having the sprite just below the window title bar.
#else
        float s = 2;
#endif
        //System::Print("screenX: %d, screenPoint.x: %f, max screenPoint.x: %f, screenY * s: %f, screenPoint.y * 2: %f, screenPoint.y * 2 + texHeight: %f\n", screenX, screenPoint.x, screenPoint.x + texWidth / s, screenY * s, screenPoint.y * 2, screenPoint.y * 2 + texHeight);
        if (screenX > screenPoint.x && screenX < screenPoint.x + texWidth / s &&
            screenY * s > screenPoint.y * s && screenY * s < screenPoint.y * s + texHeight)
        {
            System::Print("Hit game object sprite\n");
            sv->scene.Add( sv->gameObjects[ 0 ] );
            sv->gameObjects[ 0 ]->GetComponent< TransformComponent >()->SetLocalPosition( sv->gameObjects[ goIndex ]->GetComponent<TransformComponent>()->GetLocalPosition() );

            sv->selectedGameObjects.Add( sv->gameObjects[ goIndex ] );
            sv->selectedGOIndex = goIndex;
            return sv->gameObjects[ goIndex ];
        }
    }

    // Checks if the mouse hit a mesh and selects the object.
    Array< CollisionInfo > ci;
    GetColliders( sv->camera, sv->selectedGameObjects.count != 0, screenX, screenY, width, height, 200, sv->gameObjects, colTest, ci );

    if (ci.count > 0 && ci[ 0 ].go != sv->gameObjects[ 0 ])
    {
        sv->scene.Add( sv->gameObjects[ 0 ] );
        sv->gameObjects[ 0 ]->GetComponent< TransformComponent >()->SetLocalPosition( ci[ 0 ].go->GetComponent<TransformComponent>()->GetLocalPosition() );
        //System::Print( "collided with submesh: %d\n", ci[ 0 ].subMeshIndex );

        sv->selectedGameObjects.Add( ci[ 0 ].go );
        sv->selectedGOIndex = ci[ 0 ].gameObjectIndex;

        return ci[ 0 ].go;
    }

    sv->selectedGameObjects.Allocate( 0 );
    sv->scene.Remove( sv->gameObjects[ 0 ] );
    return nullptr;
}

void svUpdate( SceneView* sceneView )
{
    if (sceneView->selectedGameObjects.count > 0 && sceneView->selectedGameObjects[ 0 ]->GetComponent< TransformComponent >())
    {
        sceneView->gameObjects[ 0 ]->GetComponent< TransformComponent >()->SetLocalPosition( sceneView->selectedGameObjects[ 0 ]->GetComponent<TransformComponent>()->GetLocalPosition() );
    }
}

void svHighlightGizmo( SceneView* sv, int screenX, int screenY, int width, int height )
{
    Array< CollisionInfo > ci;

    GetColliders( sv->camera, sv->selectedGameObjects.count != 0, screenX, screenY, width, height, 200, sv->gameObjects, colTest, ci );

    const bool isGizmo = (ci.count == 0) ? false : (ci[ 0 ].go == sv->gameObjects[ 0 ]);
    const int selected = isGizmo ? ci[ 0 ].subMeshIndex : -1;

    if (selected == 0)
    {
        sv->gameObjects[ 0 ]->GetComponent< ae3d::MeshRendererComponent >()->SetMaterial( &sv->transformGizmo.xAxisMaterial, 1 );
        sv->gameObjects[ 0 ]->GetComponent< ae3d::MeshRendererComponent >()->SetMaterial( &sv->transformGizmo.yAxisMaterial, 2 );
        sv->gameObjects[ 0 ]->GetComponent< ae3d::MeshRendererComponent >()->SetMaterial( &sv->highlightMaterial, 0 );
    }
    else if (selected == 1)
    {
        sv->gameObjects[ 0 ]->GetComponent< ae3d::MeshRendererComponent >()->SetMaterial( &sv->highlightMaterial, 1 );
        sv->gameObjects[ 0 ]->GetComponent< ae3d::MeshRendererComponent >()->SetMaterial( &sv->transformGizmo.yAxisMaterial, 2 );
        sv->gameObjects[ 0 ]->GetComponent< ae3d::MeshRendererComponent >()->SetMaterial( &sv->transformGizmo.zAxisMaterial, 0 );
    }
    else if (selected == 2)
    {
        sv->gameObjects[ 0 ]->GetComponent< ae3d::MeshRendererComponent >()->SetMaterial( &sv->transformGizmo.xAxisMaterial, 1 );
        sv->gameObjects[ 0 ]->GetComponent< ae3d::MeshRendererComponent >()->SetMaterial( &sv->highlightMaterial, 2 );
        sv->gameObjects[ 0 ]->GetComponent< ae3d::MeshRendererComponent >()->SetMaterial( &sv->transformGizmo.zAxisMaterial, 0 );
    }
    else
    {
        sv->gameObjects[ 0 ]->GetComponent< ae3d::MeshRendererComponent >()->SetMaterial( &sv->transformGizmo.xAxisMaterial, 1 );
        sv->gameObjects[ 0 ]->GetComponent< ae3d::MeshRendererComponent >()->SetMaterial( &sv->transformGizmo.yAxisMaterial, 2 );
        sv->gameObjects[ 0 ]->GetComponent< ae3d::MeshRendererComponent >()->SetMaterial( &sv->transformGizmo.zAxisMaterial, 0 );
    }
}

void svHandleLeftMouseDown( SceneView* sv, int screenX, int screenY, int width, int height )
{
    Array< CollisionInfo > ci;

    GetColliders( sv->camera, sv->selectedGameObjects.count != 0, screenX, screenY, width, height, 200, sv->gameObjects, colTest, ci );

    const bool isGizmo = (ci.count == 0) ? false : (ci[ 0 ].go == sv->gameObjects[ 0 ]);
    sv->transformGizmo.selectedMesh = isGizmo ? ci[ 0 ].subMeshIndex : -1;
    //ae3d::System::Print("collision count: %d, gizmo mesh %d\n", ci.count, sv->transformGizmo.selectedMesh);

    if (!isGizmo)
    {
        sv->selectedGameObjects.Allocate( 0 );
    }
}

void svHandleLeftMouseUp( SceneView* sv )
{
    sv->transformGizmo.selectedMesh = -1;
}

bool svIsTransformGizmoSelected( SceneView* sceneView )
{
    return sceneView->transformGizmo.selectedMesh != -1;
}

bool svIsDraggingGizmo( SceneView* sv )
{
    return sv->transformGizmo.selectedMesh != -1;
}

void svHandleMouseMotion( SceneView* sv, int deltaX, int deltaY )
{
    if (sv->transformGizmo.selectedMesh != -1)
    {
#if RENDERER_VULKAN
        Vec3 delta{ deltaX / 20.0f, -deltaY / 20.0f, 0.0f };
#else
        Vec3 delta{ -deltaX / 20.0f, deltaY / 20.0f, 0.0f };
#endif
        //ae3d::System::Print("move. mesh %d\n", sv->transformGizmo.selectedMesh);
        // Mesh 0 is z, 1 is x, 2 is y.

        if (sv->transformGizmo.selectedMesh == 0)
        {
            delta.x = 0;
            delta.y = 0;
            delta.z = deltaX / 20.0f;
            float sign = Vec3::Dot( sv->camera.GetComponent< ae3d::TransformComponent >()->GetViewDirection(), Vec3( 1, 0, 0 ) );

            if (sign > 0)
            {
                delta.z = -delta.z;
            }
        }
        else if (sv->transformGizmo.selectedMesh == 1)
        {
            float sign = Vec3::Dot( sv->camera.GetComponent< ae3d::TransformComponent >()->GetViewDirection(), Vec3( 0, 0, 1 ) );

            if (sign < 0)
            {
                delta.x = -delta.x;
            }

            delta.y = 0;
        }
        else if (sv->transformGizmo.selectedMesh == 2)
        {
            delta.x = 0;
        }

        Vec3 oldPos = sv->gameObjects[ 0 ]->GetComponent< TransformComponent >()->GetLocalPosition();
        sv->gameObjects[ 0 ]->GetComponent< TransformComponent >()->SetLocalPosition( oldPos + delta );
		
        for (unsigned i = 0; i < sv->selectedGameObjects.count; ++i)
        {
            sv->selectedGameObjects[ i ]->GetComponent< TransformComponent >()->SetLocalPosition( oldPos + delta );
        }
    }
}

void svRotateCamera( SceneView* sv, float xDegrees, float yDegrees )
{
    sv->camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), xDegrees );
    sv->camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 1, 0, 0 ), yDegrees );
}

void svDeleteGameObject( SceneView* sv )
{
    if (sv->selectedGameObjects.count > 0)
    {
        sv->scene.Remove( sv->selectedGameObjects[ 0 ] );
        sv->selectedGameObjects.Allocate( 0 );
        //sv->transformGizmo.selectedMesh = -1;
        sv->scene.Remove( sv->gameObjects[ 0 ] );
        sv->gameObjects.Remove( sv->selectedGOIndex );
    }
}

void svFocusOnSelected( SceneView* sv )
{
    if (sv->selectedGameObjects.count > 0)
    {
        GameObject* go = sv->selectedGameObjects[ 0 ];
        TransformComponent* tc = go->GetComponent< TransformComponent >();
        Vec3 goPos = tc->GetWorldPosition();
        sv->camera.GetComponent<TransformComponent>()->SetLocalPosition( goPos + Vec3( 0, 0, 5 ) );
    }
}

void TransformGizmo::Init( Shader* shader, GameObject& go )
{
    translateMesh.Load( FileSystem::FileContents( "cursor_translate.ae3d" ) );
    
    xAxisMaterial.SetShader( shader );
    xAxisMaterial.SetTexture( &redTex, 0 );
    xAxisMaterial.SetDepthFunction( Material::DepthFunction::LessOrEqualWriteOn );
    float factor = -100;
    float units = 0;
    xAxisMaterial.SetDepthOffset( factor, units );
    yAxisMaterial.SetDepthOffset( factor, units );
    zAxisMaterial.SetDepthOffset( factor, units );
    
    yAxisMaterial.SetShader( shader );
    yAxisMaterial.SetTexture( &greenTex, 0 );
    yAxisMaterial.SetDepthFunction( Material::DepthFunction::LessOrEqualWriteOn );
    
    zAxisMaterial.SetShader( shader );
    zAxisMaterial.SetTexture( &blueTex, 0 );
    zAxisMaterial.SetDepthFunction( Material::DepthFunction::LessOrEqualWriteOn );

    go.AddComponent< MeshRendererComponent >();
    go.GetComponent< MeshRendererComponent >()->SetMesh( &translateMesh );
    go.GetComponent< MeshRendererComponent >()->SetMaterial( &xAxisMaterial, 1 );
    go.GetComponent< MeshRendererComponent >()->SetMaterial( &yAxisMaterial, 2 );
    go.GetComponent< MeshRendererComponent >()->SetMaterial( &zAxisMaterial, 0 );
    go.AddComponent< TransformComponent >();
    go.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 10, -50 } );
    go.SetName( "EditorGizmo" );
}

void svDrawSprites( SceneView* sv, unsigned screenWidth, unsigned screenHeight )
{
    ae3d::TransformComponent* cameraTransform = sv->camera.GetComponent<TransformComponent>();
    ae3d::CameraComponent* camera = sv->camera.GetComponent<CameraComponent>();
    const unsigned texWidth = sv->lightTex.GetWidth();
    const unsigned texHeight = sv->lightTex.GetHeight();
#if RENDERER_VULKAN
    const float screenScale = 1;
#else
    const float screenScale = 2;
#endif

    for (unsigned goIndex = 1; goIndex < sv->gameObjects.count; ++goIndex)
    {
        ae3d::TransformComponent* goTransform = sv->gameObjects[ goIndex ]->GetComponent<TransformComponent>();

        if (!goTransform)
        {
            continue;
        }
        
        const float distance = (cameraTransform->GetLocalPosition() - goTransform->GetLocalPosition()).Length();
        const float lerpDistance = 10;
        float opacity = 1;
        
        if (distance < lerpDistance)
        {
            opacity = distance / lerpDistance;
        }
        
        const Vec3 screenPoint = camera->GetScreenPoint( goTransform->GetLocalPosition(), (float)screenWidth, (float)screenHeight );
        const Vec3 viewDir = cameraTransform->GetViewDirection();
        const Vec3 lightDir = (goTransform->GetLocalPosition() - cameraTransform->GetLocalPosition()).Normalized();
        const float viewDotLight = Vec3::Dot( viewDir, lightDir ) ;
        Texture2D* sprite = &sv->goTex;

        if (sv->gameObjects[ goIndex ]->GetComponent<SpotLightComponent>() || sv->gameObjects[ goIndex ]->GetComponent<DirectionalLightComponent>() ||
            sv->gameObjects[ goIndex ]->GetComponent<PointLightComponent>())
        {
            sprite = &sv->lightTex;
        }

        if (sv->gameObjects[ goIndex ]->GetComponent<AudioSourceComponent>())
        {
            sprite = &sv->audioTex;
        }

        if (sv->gameObjects[ goIndex ]->GetComponent<CameraComponent>())
        {
            sprite = &sv->cameraTex;
        }

        if (viewDotLight <= 0 &&
            screenPoint.x > -(float)texWidth && screenPoint.y > -(float)texHeight &&
            screenPoint.x < screenWidth * screenScale && screenPoint.y < screenHeight * screenScale)
        {
            //const float spriteScale = screenHeight / distance;
#if RENDERER_VULKAN
            float x = screenPoint.x;
            float y = screenHeight /* screenScale*/ - screenPoint.y;
#else
            float x = (int)screenPoint.x * screenScale;
            float y = (int)screenPoint.y * screenScale;
#endif
            //printf( "draw: %.2f, %.2f to %.2f, %.2f\n", x, y, x + texWidth, y + texHeight );
            ae3d::System::Draw( sprite, x, y, texWidth, texHeight,
                                screenWidth * screenScale, screenHeight * screenScale, Vec4( 1, 1, 1, opacity ), ae3d::System::BlendMode::Off );
        }
    }

    System::DrawLines( sv->lineHandle, camera->GetView(), camera->GetProjection(), screenWidth * screenScale, screenHeight * screenScale );
}
