#include "Scene.hpp"
#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <cmath>
#include "AudioSourceComponent.hpp"
#include "CameraComponent.hpp"
#include "DirectionalLightComponent.hpp"
#include "FileSystem.hpp"
#include "Frustum.hpp"
#include "GameObject.hpp"
#include "GfxDevice.hpp"
#include "Matrix.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "MeshRendererComponent.hpp"
#include "RenderTexture.hpp"
#include "SpriteRendererComponent.hpp"
#include "TextRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "Renderer.hpp"
#include "System.hpp"

using namespace ae3d;
extern ae3d::Renderer renderer;

namespace MathUtil
{
    void GetMinMax( const std::vector< Vec3 >& aPoints, Vec3& outMin, Vec3& outMax );
    
    float Floor( float f )
    {
        return std::floor( f );
    }
}

namespace SceneGlobal
{
    GameObject shadowCamera;
    bool isShadowCameraCreated = false;
    Matrix44 shadowCameraViewMatrix;
    Matrix44 shadowCameraProjectionMatrix;
}

void SetupCameraForDirectionalShadowCasting( const Vec3& lightDirection, const Frustum& eyeFrustum, const Vec3& sceneAABBmin, const Vec3& sceneAABBmax,
                                             ae3d::CameraComponent& outCamera, ae3d::TransformComponent& outCameraTransform )
{
    System::Assert( outCamera.GetTargetTexture() != nullptr, "Shadow camera needs target texture" );
    System::Assert( lightDirection.Length() > 0.9f && lightDirection.Length() < 1.1f, "Light dir must be normalized" );

    const Vec3 viewFrustumCentroid = eyeFrustum.Centroid();

    // Start at the centroid, and move back in the opposite direction of the light
    // by an amount equal to the camera's farClip. This is the temporary working position for the light.
    const Vec3 shadowCameraPosition = viewFrustumCentroid - lightDirection * eyeFrustum.FarClipPlane();
    
    outCameraTransform.LookAt( shadowCameraPosition, viewFrustumCentroid, Vec3( 0, 1, 0 ) );
    
    Matrix44 view;
    outCameraTransform.GetLocalRotation().GetMatrix( view );
    Matrix44 translation;
    translation.Translate( -outCameraTransform.GetLocalPosition() );
    Matrix44::Multiply( translation, view, view );
    outCamera.SetView( view );

    const Matrix44& shadowCameraView = outCamera.GetView();
    
    // Transforms view camera frustum points to shadow camera space.
    std::vector< Vec3 > viewFrustumLS( 8 );
    
    Matrix44::TransformPoint( eyeFrustum.NearTopLeft(), shadowCameraView, &viewFrustumLS[ 0 ] );
    Matrix44::TransformPoint( eyeFrustum.NearTopRight(), shadowCameraView, &viewFrustumLS[ 1 ] );
    Matrix44::TransformPoint( eyeFrustum.NearBottomLeft(), shadowCameraView, &viewFrustumLS[ 2 ] );
    Matrix44::TransformPoint( eyeFrustum.NearBottomRight(), shadowCameraView, &viewFrustumLS[ 3 ] );
    
    Matrix44::TransformPoint( eyeFrustum.FarTopLeft(), shadowCameraView, &viewFrustumLS[ 4 ] );
    Matrix44::TransformPoint( eyeFrustum.FarTopRight(), shadowCameraView, &viewFrustumLS[ 5 ] );
    Matrix44::TransformPoint( eyeFrustum.FarBottomLeft(), shadowCameraView, &viewFrustumLS[ 6 ] );
    Matrix44::TransformPoint( eyeFrustum.FarBottomRight(), shadowCameraView, &viewFrustumLS[ 7 ] );
    
    // Gets light-space view frustum extremes.
    Vec3 viewMinLS, viewMaxLS;
    MathUtil::GetMinMax( viewFrustumLS, viewMinLS, viewMaxLS );
    
    // Transforms scene's AABB to light-space.
    Vec3 sceneAABBminLS, sceneAABBmaxLS;
    const Vec3 min = sceneAABBmin;
    const Vec3 max = sceneAABBmax;
    
    const Vec3 sceneCorners[] =
    {
        Vec3( min.x, min.y, min.z ),
        Vec3( max.x, min.y, min.z ),
        Vec3( min.x, max.y, min.z ),
        Vec3( min.x, min.y, max.z ),
        Vec3( max.x, max.y, min.z ),
        Vec3( min.x, max.y, max.z ),
        Vec3( max.x, max.y, max.z ),
        Vec3( max.x, min.y, max.z )
    };
    
    std::vector< Vec3 > sceneAABBLS( 8 );
    
    for (int i = 0; i < 8; ++i)
    {
        Matrix44::TransformPoint( sceneCorners[ i ], shadowCameraView, &sceneAABBLS[ i ] );
    }
    
    MathUtil::GetMinMax( sceneAABBLS, sceneAABBminLS, sceneAABBmaxLS );
    
    // Use world volume for near plane.
    viewMaxLS.z = sceneAABBmaxLS.z > viewMaxLS.z ? sceneAABBmaxLS.z : viewMaxLS.z;
   
    outCamera.SetProjectionType( ae3d::CameraComponent::ProjectionType::Orthographic );
    outCamera.SetProjection( viewMinLS.x, viewMaxLS.x, viewMinLS.y, viewMaxLS.y, -viewMaxLS.z, -viewMinLS.z );
}

void ae3d::Scene::Add( GameObject* gameObject )
{
    for (const auto& go : gameObjects)
    {
        if (go == gameObject)
        {
            return;
        }
    }

    if (nextFreeGameObject == gameObjects.size())
    {
        gameObjects.resize( gameObjects.size() + 10 );
    }

    gameObjects[ nextFreeGameObject++ ] = gameObject;

    CameraComponent* camera = gameObject->GetComponent<CameraComponent>();

    if (camera && mainCamera == nullptr)
    {
        mainCamera = gameObject;
    }
}

void ae3d::Scene::Remove( GameObject* gameObject )
{
    for (std::size_t i = 0; i < gameObjects.size(); ++i)
    {
        if (gameObject == gameObjects[ i ])
        {
            gameObjects.erase( std::begin( gameObjects ) + i );
            return;
        }
    }
}

void ae3d::Scene::Render()
{
    GenerateAABB();
    GfxDevice::ResetFrameStatistics();

    if (mainCamera == nullptr || mainCamera->GetComponent<CameraComponent>() == nullptr)
    {
        return;
    }
    
    std::vector< GameObject* > rtCameras;
    rtCameras.reserve( gameObjects.size() / 4 );
    std::vector< GameObject* > cameras;
    cameras.reserve( gameObjects.size() / 4 );
    
    for (auto gameObject : gameObjects)
    {
        if (gameObject == nullptr)
        {
            continue;
        }

        auto cameraComponent = gameObject->GetComponent<CameraComponent>();
        
        if (cameraComponent && cameraComponent->GetTargetTexture() != nullptr)
        {
            rtCameras.push_back( gameObject );
        }
        if (cameraComponent && cameraComponent->GetTargetTexture() == nullptr)
        {
            cameras.push_back( gameObject );
        }
    }
    
    for (auto rtCamera : rtCameras)
    {
        auto transform = rtCamera->GetComponent< TransformComponent >();

        if (transform && !rtCamera->GetComponent< CameraComponent >()->GetTargetTexture()->IsCube())
        {
            RenderWithCamera( rtCamera, 0 );
        }
        else if (transform && rtCamera->GetComponent< CameraComponent >()->GetTargetTexture()->IsCube())
        {
            for (int cubeMapFace = 0; cubeMapFace < 6; ++cubeMapFace)
            {
                const float scale = 2000;

                static const Vec3 directions[ 6 ] =
                {
                    Vec3(  1,  0,  0 ) * scale, // posX
                    Vec3( -1,  0,  0 ) * scale, // negX
                    Vec3(  0,  1,  0 ) * scale, // posY
                    Vec3(  0, -1,  0 ) * scale, // negY
                    Vec3(  0,  0,  1 ) * scale, // posZ
                    Vec3(  0,  0, -1 ) * scale  // negZ
                };
                
                static const Vec3 ups[ 6 ] =
                {
                    Vec3( 0, -1,  0 ),
                    Vec3( 0, -1,  0 ),
                    Vec3( 0,  0,  1 ),
                    Vec3( 0,  0, -1 ),
                    Vec3( 0, -1,  0 ),
                    Vec3( 0, -1,  0 )
                };
                
                transform->LookAt( transform->GetLocalPosition(), transform->GetLocalPosition() + directions[ cubeMapFace ], ups[ cubeMapFace ] );
                RenderWithCamera( rtCamera, cubeMapFace );
            }
        }
    }

    for (auto camera : cameras)
    {
        if (camera != mainCamera && camera->GetComponent<TransformComponent>())
        {
            RenderWithCamera( camera, 0 );
        }
    }
    
    CameraComponent* camera = mainCamera->GetComponent<CameraComponent>();
    ae3d::System::Assert( mainCamera->GetComponent<CameraComponent>()->GetTargetTexture() == nullptr, "main camera must not have a texture target" );

    unsigned debugShadowFBO = 0;
    
    if (camera != nullptr && mainCamera->GetComponent<TransformComponent>())
    {
        TransformComponent* mainCameraTransform = mainCamera->GetComponent<TransformComponent>();

        // Shadow pass
        for (auto go : gameObjects)
        {
            if (!go)
            {
                continue;
            }
            
            auto lightTransform = go->GetComponent<TransformComponent>();
            
            if (lightTransform && go->GetComponent<DirectionalLightComponent>() && go->GetComponent<DirectionalLightComponent>()->CastsShadow())
            {
                Frustum eyeFrustum;
                
                if (camera->GetProjectionType() == CameraComponent::ProjectionType::Perspective)
                {
                    eyeFrustum.SetProjection( camera->GetFovDegrees(), camera->GetAspect(), camera->GetNear(), camera->GetFar() );
                }
                else
                {
                    eyeFrustum.SetProjection( camera->GetLeft(), camera->GetRight(), camera->GetBottom(), camera->GetTop(), camera->GetNear(), camera->GetFar() );
                }
                
                Matrix44 eyeView;
                mainCameraTransform->GetLocalRotation().GetMatrix( eyeView );
                Matrix44 translation;
                translation.Translate( -mainCameraTransform->GetLocalPosition() );
                Matrix44::Multiply( translation, eyeView, eyeView );

                const Vec3 eyeViewDir = Vec3( eyeView.m[2], eyeView.m[6], eyeView.m[10] ).Normalized();
                eyeFrustum.Update( mainCameraTransform->GetLocalPosition(), eyeViewDir );

                if (!SceneGlobal::isShadowCameraCreated)
                {
                    SceneGlobal::shadowCamera.AddComponent< CameraComponent >();
                    SceneGlobal::shadowCamera.AddComponent< TransformComponent >();
                    SceneGlobal::isShadowCameraCreated = true;
                }
                
                SceneGlobal::shadowCamera.GetComponent< CameraComponent >()->SetTargetTexture( &go->GetComponent<DirectionalLightComponent>()->shadowMap );
                
                SetupCameraForDirectionalShadowCasting( lightTransform->GetViewDirection(), eyeFrustum, aabbMin, aabbMax, *SceneGlobal::shadowCamera.GetComponent< CameraComponent >(), *SceneGlobal::shadowCamera.GetComponent< TransformComponent >() );
                
                RenderShadowsWithCamera( &SceneGlobal::shadowCamera, 0 );
                
                Material::SetGlobalRenderTexture( "_ShadowMap", &go->GetComponent<DirectionalLightComponent>()->shadowMap );
                
                debugShadowFBO = go->GetComponent<DirectionalLightComponent>()->shadowMap.GetFBO();
            }
        }
        
        RenderWithCamera( mainCamera, 0 );
    }
    
    //GfxDevice::DebugBlitFBO( debugShadowFBO, 256, 256 );
}

void ae3d::Scene::RenderWithCamera( GameObject* cameraGo, int cubeMapFace )
{
    ae3d::System::Assert( 0 <= cubeMapFace && cubeMapFace < 6, "invalid cube map face" );

    CameraComponent* camera = cameraGo->GetComponent< CameraComponent >();
    GfxDevice::SetRenderTarget( camera->GetTargetTexture(), cubeMapFace );
    
    const Vec3 color = camera->GetClearColor();
    GfxDevice::SetClearColor( color.x, color.y, color.z );

    if (camera->GetClearFlag() == CameraComponent::ClearFlag::DepthAndColor)
    {
        GfxDevice::ClearScreen( GfxDevice::ClearFlags::Color | GfxDevice::ClearFlags::Depth );
    }
    
    Matrix44 view;

    if (skybox != nullptr)
    {
        auto cameraTrans = cameraGo->GetComponent< TransformComponent >();
        cameraTrans->GetLocalRotation().GetMatrix( view );
#if OCULUS_RIFT
        Matrix44 vrView = cameraTrans->GetVrView();
        // Cancels translation.
        vrView.m[ 14 ] = 0;
        vrView.m[ 13 ] = 0;
        vrView.m[ 12 ] = 0;

        camera->SetView( vrView );
#else
        camera->SetView( view );
#endif
        renderer.RenderSkybox( skybox, *camera );
    }

    auto cameraTransform = cameraGo->GetComponent< TransformComponent >();
    
    // TODO: Maybe add a VR flag into camera to select between HMD and normal pose.
#if OCULUS_RIFT
    view = cameraGo->GetComponent< TransformComponent >()->GetVrView();
#else
    cameraTransform->GetLocalRotation().GetMatrix( view );
    Matrix44 translation;
    translation.Translate( -cameraTransform->GetLocalPosition() );
    Matrix44::Multiply( translation, view, view );
#endif
    
    Frustum frustum;
    
    if (camera->GetProjectionType() == CameraComponent::ProjectionType::Perspective)
    {
        frustum.SetProjection( camera->GetFovDegrees(), camera->GetAspect(), camera->GetNear(), camera->GetFar() );
    }
    else
    {
        frustum.SetProjection( camera->GetLeft(), camera->GetRight(), camera->GetBottom(), camera->GetTop(), camera->GetNear(), camera->GetFar() );
    }
    
    const Vec3 viewDir = Vec3( view.m[2], view.m[6], view.m[10] ).Normalized();
    frustum.Update( cameraTransform->GetLocalPosition(), viewDir );

    std::vector< unsigned > gameObjectsWithMeshRenderer;
    gameObjectsWithMeshRenderer.reserve( gameObjects.size() );
    int i = -1;
    
    for (auto gameObject : gameObjects)
    {
        ++i;
        
        if (gameObject == nullptr)
        {
            continue;
        }
        
        auto transform = gameObject->GetComponent< TransformComponent >();
        auto spriteRenderer = gameObject->GetComponent< SpriteRendererComponent >();

        GfxDevice::SetBackFaceCulling( false );
        
        if (spriteRenderer)
        {
            Matrix44 projectionModel;
            Matrix44::Multiply( transform ? transform->GetLocalMatrix() : Matrix44::identity, camera->GetProjection(), projectionModel );
            spriteRenderer->Render( projectionModel.m );
        }
        
        auto textRenderer = gameObject->GetComponent< TextRendererComponent >();
        
        if (textRenderer)
        {
            Matrix44 projectionModel;
            Matrix44::Multiply( transform ? transform->GetLocalMatrix() : Matrix44::identity, camera->GetProjection(), projectionModel );
            textRenderer->Render( projectionModel.m );
        }
        
        auto meshRenderer = gameObject->GetComponent< MeshRendererComponent >();

        if (meshRenderer)
        {
            gameObjectsWithMeshRenderer.push_back( i );
        }
    }

    auto meshSorterByMesh = [&](unsigned j, unsigned k) { return gameObjects[ j ]->GetComponent< MeshRendererComponent >()->GetMesh() ==
                                                                 gameObjects[ k ]->GetComponent< MeshRendererComponent >()->GetMesh(); };
    std::sort( std::begin( gameObjectsWithMeshRenderer ), std::end( gameObjectsWithMeshRenderer ), meshSorterByMesh );
    
    for (auto j : gameObjectsWithMeshRenderer)
    {
        auto transform = gameObjects[ j ]->GetComponent< TransformComponent >();
        auto meshLocalToWorld = transform ? transform->GetLocalMatrix() : Matrix44::identity;
            
        Matrix44 mvp;
        Matrix44::Multiply( meshLocalToWorld, view, mvp );
        Matrix44::Multiply( mvp, camera->GetProjection(), mvp );
        
        gameObjects[ j ]->GetComponent< MeshRendererComponent >()->Render( mvp, frustum, meshLocalToWorld, nullptr );
    }
    
    GfxDevice::ErrorCheck( "Scene render end" );
}

void ae3d::Scene::RenderShadowsWithCamera( GameObject* cameraGo, int cubeMapFace )
{
    CameraComponent* camera = cameraGo->GetComponent< CameraComponent >();
    GfxDevice::SetRenderTarget( camera->GetTargetTexture(), cubeMapFace );
    
    const Vec3 color = camera->GetClearColor();
    GfxDevice::SetClearColor( color.x, color.y, color.z );
    
    if (camera->GetClearFlag() == CameraComponent::ClearFlag::DepthAndColor)
    {
        GfxDevice::ClearScreen( GfxDevice::ClearFlags::Color | GfxDevice::ClearFlags::Depth );
    }
    
    Matrix44 view;
    auto cameraTransform = cameraGo->GetComponent< TransformComponent >();
    cameraTransform->GetLocalRotation().GetMatrix( view );
    Matrix44 translation;
    translation.Translate( -cameraTransform->GetLocalPosition() );
    Matrix44::Multiply( translation, view, view );
    
    SceneGlobal::shadowCameraViewMatrix = view;
    SceneGlobal::shadowCameraProjectionMatrix = camera->GetProjection();
    
    Frustum frustum;
    
    if (camera->GetProjectionType() == CameraComponent::ProjectionType::Perspective)
    {
        frustum.SetProjection( camera->GetFovDegrees(), camera->GetAspect(), camera->GetNear(), camera->GetFar() );
    }
    else
    {
        frustum.SetProjection( camera->GetLeft(), camera->GetRight(), camera->GetBottom(), camera->GetTop(), camera->GetNear(), camera->GetFar() );
    }
    
    const Vec3 viewDir = Vec3( view.m[2], view.m[6], view.m[10] ).Normalized();
    frustum.Update( cameraTransform->GetLocalPosition(), viewDir );
    
    std::vector< unsigned > gameObjectsWithMeshRenderer;
    gameObjectsWithMeshRenderer.reserve( gameObjects.size() );
    int i = -1;
    
    for (auto gameObject : gameObjects)
    {
        ++i;
        
        if (gameObject == nullptr)
        {
            continue;
        }
        
        auto meshRenderer = gameObject->GetComponent< MeshRendererComponent >();
        
        if (meshRenderer)
        {
            gameObjectsWithMeshRenderer.push_back( i );
        }
    }
    
    auto meshSorterByMesh = [&](unsigned j, unsigned k) { return gameObjects[ j ]->GetComponent< MeshRendererComponent >()->GetMesh() ==
        gameObjects[ k ]->GetComponent< MeshRendererComponent >()->GetMesh(); };
    std::sort( std::begin( gameObjectsWithMeshRenderer ), std::end( gameObjectsWithMeshRenderer ), meshSorterByMesh );
    
    for (auto j : gameObjectsWithMeshRenderer)
    {
        auto transform = gameObjects[ j ]->GetComponent< TransformComponent >();
        auto meshLocalToWorld = transform ? transform->GetLocalMatrix() : Matrix44::identity;
        
        Matrix44 mvp;
        Matrix44::Multiply( meshLocalToWorld, view, mvp );
        Matrix44::Multiply( mvp, camera->GetProjection(), mvp );

        gameObjects[ j ]->GetComponent< MeshRendererComponent >()->Render( mvp, frustum, meshLocalToWorld, &renderer.builtinShaders.momentsShader );
    }
    
    GfxDevice::ErrorCheck( "Scene render shadows end" );
}

void ae3d::Scene::SetSkybox( const TextureCube* skyTexture )
{
    skybox = skyTexture;

    if (skybox != nullptr && !renderer.IsSkyboxGenerated())
    {
        renderer.GenerateSkybox();
    }
}

std::string ae3d::Scene::GetSerialized() const
{
    std::string outSerialized;

    for (auto gameObject : gameObjects)
    {
        if (gameObject == nullptr)
        {
            continue;
        }

        outSerialized += gameObject->GetSerialized();
        
        // TODO: Try to DRY.
        if (gameObject->GetComponent<TransformComponent>())
        {
            outSerialized += gameObject->GetComponent<TransformComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<CameraComponent>())
        {
            outSerialized += gameObject->GetComponent<CameraComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<TextRendererComponent>())
        {
            outSerialized += gameObject->GetComponent<TextRendererComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<AudioSourceComponent>())
        {
            outSerialized += gameObject->GetComponent<AudioSourceComponent>()->GetSerialized();
        }
    }
    return outSerialized;
}

ae3d::Scene::DeserializeResult ae3d::Scene::Deserialize( const FileSystem::FileContentsData& serialized, std::vector< GameObject >& outGameObjects ) const
{
    // TODO: It would be better to store the token strings into somewhere accessible to GetSerialized() to prevent typos etc.

    outGameObjects.clear();

    std::stringstream stream( std::string( std::begin( serialized.data ), std::end( serialized.data ) ) );
    std::string line;
    
    while (!stream.eof())
    {
        std::getline( stream, line );
        std::stringstream lineStream( line );
        std::string token;
        lineStream >> token;
        
        if (token == "gameobject")
        {
            outGameObjects.push_back( GameObject() );
            std::string name;
            lineStream >> name;
            outGameObjects.back().SetName( name.c_str() );
        }

        if (token == "dirlight")
        {
            outGameObjects.back().AddComponent< DirectionalLightComponent >();
        }

        if (token == "shadow")
        {
            int enabled;
            lineStream >> enabled;
            outGameObjects.back().GetComponent< DirectionalLightComponent >()->SetCastShadow( enabled ? true : false, 512 );
        }

        if (token == "camera")
        {
            outGameObjects.back().AddComponent< CameraComponent >();
        }

        if (token == "ortho")
        {
            float x, y, width, height, nearp, farp;
            lineStream >> x >> y >> width >> height >> nearp >> farp;
            outGameObjects.back().GetComponent< CameraComponent >()->SetProjection( x, y, width, height, nearp, farp );
        }

        if (token == "persp")
        {
            float fov, aspect, nearp, farp;
            lineStream >> fov >> aspect >> nearp >> farp;
            outGameObjects.back().GetComponent< CameraComponent >()->SetProjection( fov, aspect, nearp, farp );
        }

        if (token == "projection")
        {
            std::string type;
            lineStream >> type;
            
            if (type == "orthographic")
            {
                outGameObjects.back().GetComponent< CameraComponent >()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Orthographic );
            }
            else if (type == "perspective")
            {
                outGameObjects.back().GetComponent< CameraComponent >()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Perspective );
            }
        }

        if (token == "clearcolor")
        {
            float red, green, blue;
            lineStream >> red >> green >> blue;
            outGameObjects.back().GetComponent< CameraComponent >()->SetClearColor( { red, green, blue } );
        }

        if (token == "transform")
        {
            outGameObjects.back().AddComponent< TransformComponent >();
        }

        if (token == "position")
        {
            float x, y, z;
            lineStream >> x >> y >> z;
            outGameObjects.back().GetComponent< TransformComponent >()->SetLocalPosition( { x, y, z } );
        }

        if (token == "rotation")
        {
            float x, y, z, s;
            lineStream >> x >> y >> z >> s;
            outGameObjects.back().GetComponent< TransformComponent >()->SetLocalRotation( { { x, y, z }, s } );
        }
    }
    
    return DeserializeResult::Success;
}

void ae3d::Scene::GenerateAABB()
{
    const float maxValue = 99999999.0f;
    aabbMin = {  maxValue,  maxValue,  maxValue };
    aabbMax = { -maxValue, -maxValue, -maxValue };
    
    for (auto& o : gameObjects)
    {
        if (!o)
        {
            continue;
        }

        auto meshRenderer = o->GetComponent< ae3d::MeshRendererComponent >();
        auto meshTransform = o->GetComponent< ae3d::TransformComponent >();

        if (meshRenderer == nullptr || meshTransform == nullptr)
        {
            continue;
        }
        
        Vec3 oAABBmin = meshRenderer->GetMesh()->GetAABBMin();
        Vec3 oAABBmax = meshRenderer->GetMesh()->GetAABBMax();
        
        Matrix44::TransformPoint( oAABBmin, meshTransform->GetLocalMatrix(), &oAABBmin );
        Matrix44::TransformPoint( oAABBmax, meshTransform->GetLocalMatrix(), &oAABBmax );
        
        if (oAABBmin.x < aabbMin.x)
        {
            aabbMin.x = oAABBmin.x;
        }
        
        if (oAABBmin.y < aabbMin.y)
        {
            aabbMin.y = oAABBmin.y;
        }
        
        if (oAABBmin.z < aabbMin.z)
        {
            aabbMin.z = oAABBmin.z;
        }
        
        if (oAABBmax.x > aabbMax.x)
        {
            aabbMax.x = oAABBmax.x;
        }
        
        if (oAABBmax.y > aabbMax.y)
        {
            aabbMax.y = oAABBmax.y;
        }
        
        if (oAABBmax.z > aabbMax.z)
        {
            aabbMax.z = oAABBmax.z;
        }
    }
}
