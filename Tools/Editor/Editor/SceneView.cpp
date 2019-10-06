// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "SceneView.hpp"
#include "Array.hpp"
#include "CameraComponent.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "MeshRendererComponent.hpp"
#include "Texture2D.hpp"
#include "TransformComponent.hpp"
#include "Scene.hpp"
#include "Shader.hpp"
#include "System.hpp"
#include "Vec3.hpp"
#include <string>
#include <vector>
#include <math.h>
#include <stdio.h>

using namespace ae3d;

struct TransformGizmo
{
    void Init( Shader* shader, GameObject& go );
    
    Mesh mesh;
    
    Mesh translateMesh;
    Material xAxisMaterial;
    Material yAxisMaterial;
    Material zAxisMaterial;

	int selectedMesh = 0;
};

struct SceneView
{
    Array< GameObject* > gameObjects;
    Array< GameObject* > selectedGameObjects;
    GameObject camera;
    Scene scene;
    Shader unlitShader;
    TransformGizmo transformGizmo;
    Texture2D lightTex;
    Texture2D cameraTex;
    Matrix44 lineView;
    Matrix44 lineProjection;
    int lineHandle = 0;
    int selectedGOIndex = 0;

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

void GetColliders( GameObject& camera, int screenX, int screenY, int width, int height, float maxDistance, Array< GameObject* >& gameObjects, CollisionTest collisionTest, Array< CollisionInfo >& outColliders )
{
    Vec3 rayOrigin, rayTarget;
    ScreenPointToRay( screenX, screenY, (float)width, (float)height, camera, rayOrigin, rayTarget );
    
    // Collects meshes that collide with the ray.
    for (unsigned i = 0; i < gameObjects.count; ++i)
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
            collisionInfo.gameObjectIndex = i;

            for (unsigned subMeshIndex = 0; subMeshIndex < meshRenderer->GetMesh()->GetSubMeshCount(); ++subMeshIndex)
            {
                Vec3 subMeshMin, subMeshMax;
                Matrix44::TransformPoint( meshRenderer->GetMesh()->GetSubMeshAABBMin( subMeshIndex ), meshLocalToWorld, &subMeshMin );
                Matrix44::TransformPoint( meshRenderer->GetMesh()->GetSubMeshAABBMax( subMeshIndex ), meshLocalToWorld, &subMeshMax );

                Array< Vec3 > triangles;
                meshRenderer->GetMesh()->GetSubMeshFlattenedTriangles( subMeshIndex, triangles );
                const float subMeshDistance = collisionTest == CollisionTest::AABB ? IntersectRayAABB( rayOrigin, rayTarget, subMeshMin, subMeshMax )
                                                                                   : IntersectRayTriangles( rayOrigin, rayTarget, triangles.elements, triangles.count );

                //System::Print("distance to submesh %d: %f. rayOrigin: %.2f, %.2f, %.2f, rayTarget: %.2f, %.2f, %.2f\n", subMeshIndex, subMeshDistance, rayOrigin.x, rayOrigin.y, rayOrigin.z, rayTarget.x, rayTarget.y, rayTarget.z);
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

void svInit( SceneView** sv, int width, int height )
{
    *sv = new SceneView();
    
    (*sv)->camera.AddComponent< CameraComponent >();
    (*sv)->camera.GetComponent< CameraComponent >()->SetProjectionType( CameraComponent::ProjectionType::Perspective );
    (*sv)->camera.GetComponent< CameraComponent >()->SetProjection( 45, (float)width / (float)height, 1, 400 );
    (*sv)->camera.GetComponent< CameraComponent >()->SetClearColor( Vec3( 0.1f, 0.1f, 0.1f ) );
    (*sv)->camera.GetComponent< CameraComponent >()->SetClearFlag( CameraComponent::ClearFlag::DepthAndColor );
    (*sv)->camera.AddComponent< TransformComponent >();
    (*sv)->camera.GetComponent< TransformComponent >()->LookAt( { 0, 0, 0 }, { 0, 0, 100 }, { 0, 1, 0 } );
    (*sv)->camera.SetName( "EditorCamera" );
    
    (*sv)->scene.Add( &(*sv)->camera );

    (*sv)->unlitShader.Load( "unlit_vertex", "unlit_fragment",
                      FileSystem::FileContents( "unlit_vert.obj" ), FileSystem::FileContents( "unlit_frag.obj" ),
                      FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );

    (*sv)->gameObjects.Add( new GameObject() );
    (*sv)->transformGizmo.Init( &(*sv)->unlitShader, *(*sv)->gameObjects[ 0 ] );

    // Test content
    
    (*sv)->gliderTex.Load( FileSystem::FileContents( "glider.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );
    (*sv)->cameraTex.Load( FileSystem::FileContents( "camera.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );
    (*sv)->lightTex.Load( FileSystem::FileContents( "light.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );

    (*sv)->material.SetShader( &(*sv)->unlitShader );
    (*sv)->material.SetTexture( &(*sv)->gliderTex, 0 );
    (*sv)->material.SetBackFaceCulling( true );

    (*sv)->cubeMesh.Load( FileSystem::FileContents( "textured_cube.ae3d" ) );
    
    (*sv)->gameObjects.Add( new GameObject() );
    (*sv)->gameObjects[ 1 ]->AddComponent< MeshRendererComponent >();
    (*sv)->gameObjects[ 1 ]->GetComponent< MeshRendererComponent >()->SetMesh( &(*sv)->cubeMesh );
    (*sv)->gameObjects[ 1 ]->GetComponent< MeshRendererComponent >()->SetMaterial( &(*sv)->material, 0 );
    (*sv)->gameObjects[ 1 ]->AddComponent< TransformComponent >();
    (*sv)->gameObjects[ 1 ]->GetComponent< TransformComponent >()->SetLocalPosition( { 0, 0, -20 } );
    (*sv)->gameObjects[ 1 ]->SetName( "cube" );
    (*sv)->scene.Add( (*sv)->gameObjects[ 1 ] );

    (*sv)->lineProjection.MakeProjection( 0, width, height, 0, 0, 1 );
    constexpr unsigned LineCount = 40;
    Vec3 lines[ LineCount ];
    
    for (unsigned i = 0; i < LineCount / 2; ++i)
    {
        lines[ i * 2 + 0 ] = Vec3( i, 0, -5 );
        lines[ i * 2 + 1 ] = Vec3( i, 0, -15 );
    }
    
    (*sv)->lineHandle = System::CreateLineBuffer( lines, LineCount, Vec3( 1, 1, 1 ) );

    // Test code
    (*sv)->transformGizmo.xAxisMaterial.SetTexture( &(*sv)->gliderTex, 0 );
    (*sv)->transformGizmo.yAxisMaterial.SetTexture( &(*sv)->gliderTex, 0 );
    (*sv)->transformGizmo.zAxisMaterial.SetTexture( &(*sv)->gliderTex, 0 );
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

void svBeginRender( SceneView* sv )
{
    sv->scene.Render();
}

void svEndRender( SceneView* sv )
{
    sv->scene.EndFrame();
}

void svLoadScene( SceneView* sv, const ae3d::FileSystem::FileContentsData& contents )
{
    std::vector< ae3d::GameObject > gos;
    std::map< std::string, class Texture2D* > texture2Ds;
    std::map< std::string, class Material* > materials;
    Array< class Mesh* > meshes;
    Scene::DeserializeResult result = sv->scene.Deserialize( contents, gos, texture2Ds, materials, meshes );

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

void svSaveScene( SceneView* sv, char* path )
{
    const std::string sceneStr = sv->scene.GetSerialized();
    FILE* f = fopen( path, "wb" );
    if (!f)
    {
        System::Print( "Could not open file for saving: %s\n", path );
        return;
    }
    
    fprintf( f, "%s\n", sceneStr.c_str() );
    fclose( f );
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
        const Vec3 screenPoint = camera->GetScreenPoint( goTransform->GetLocalPosition(), (float)width, (float)height );
        System::Print("screenX: %d, screenY: %d, screenPoint: %f, %f\n", screenX, screenY, screenPoint.x, screenPoint.y);
        
        if (goTransform && screenX > screenPoint.x - texWidth / 2 && screenX < screenPoint.x + texWidth / 2 &&
                         screenY > screenPoint.y - texHeight / 2 && screenY < screenPoint.y + texHeight / 2)
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
    GetColliders( sv->camera, screenX, screenY, width, height, 200, sv->gameObjects, CollisionTest::Triangles, ci );

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

void svHandleLeftMouseDown( SceneView* sv, int screenX, int screenY, int width, int height )
{
    Array< CollisionInfo > ci;

    GetColliders( sv->camera, screenX, screenY, width, height, 200, sv->gameObjects, CollisionTest::Triangles, ci );

    const bool isGizmo = (ci.count == 0) ? false : (ci[ 0 ].go == sv->gameObjects[ 0 ]);
    sv->transformGizmo.selectedMesh = isGizmo ? ci[ 0 ].subMeshIndex : -1;
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
        const Vec3 delta{ deltaX / 20.0f, -deltaY / 20.0f, 0.0f };
#else
        const Vec3 delta{ -deltaX / 20.0f, deltaY / 20.0f, 0.0f };
#endif
        const Vec3 oldPos = sv->gameObjects[ 0 ]->GetComponent< TransformComponent >()->GetLocalPosition();
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
        sv->camera.GetComponent<TransformComponent>()->LookAt( goPos, goPos + Vec3( 0, 0, 5 ), Vec3( 0, 1, 0 ) );
    }
}

void TransformGizmo::Init( Shader* shader, GameObject& go )
{
    translateMesh.Load( FileSystem::FileContents( "cursor_translate.ae3d" ) );
    
    xAxisMaterial.SetShader( shader );
    //xAxisMaterial.SetTexture( &translateTex, 0 );
    xAxisMaterial.SetBackFaceCulling( true );
    xAxisMaterial.SetDepthFunction( Material::DepthFunction::LessOrEqualWriteOn );
    float factor = -100;
    float units = 0;
    xAxisMaterial.SetDepthOffset( factor, units );
    yAxisMaterial.SetDepthOffset( factor, units );
    zAxisMaterial.SetDepthOffset( factor, units );
    
    yAxisMaterial.SetShader( shader );
    //yAxisMaterial.SetTexture( &translateTex, 0 );
    yAxisMaterial.SetBackFaceCulling( true );
    yAxisMaterial.SetDepthFunction( Material::DepthFunction::LessOrEqualWriteOn );
    
    zAxisMaterial.SetShader( shader );
    //zAxisMaterial.SetTexture( &translateTex, 0 );
    zAxisMaterial.SetBackFaceCulling( true );
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
        const float screenScale = 2;
        
        if (viewDotLight <= 0 &&
            screenPoint.x > -(float)texWidth && screenPoint.y > -(float)texHeight &&
            screenPoint.x < screenWidth * screenScale && screenPoint.y < screenHeight * screenScale)
        {
            //const float spriteScale = screenHeight / distance;
#if RENDERER_VULKAN
            float x = (int)screenPoint.x;
            float y = screenHeight /* screenScale*/ -  (int)screenPoint.y;
#else
            float x = (int)screenPoint.x * screenScale;
            float y = (int)screenPoint.y * screenScale;
#endif
            ae3d::System::Draw( &sv->lightTex, x, y, texWidth, texHeight,
                                screenWidth * screenScale, screenHeight * screenScale, Vec4( 1, 1, 1, 1 ), ae3d::System::BlendMode::Off );
        }
    }
    
    System::DrawLines( sv->lineHandle, camera->GetView(), camera->GetProjection() );
}
