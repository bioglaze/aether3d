#include "Scene.hpp"
#include <algorithm>
#include <locale>
#include <string>
#include <sstream>
#include <vector>
#include <cmath>
#include "AudioSourceComponent.hpp"
#include "AudioSystem.hpp"
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
#include "PointLightComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "SpotLightComponent.hpp"
#include "Statistics.hpp"
#include "TextRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "Texture2D.hpp"
#include "Renderer.hpp"
#include "System.hpp"
#if defined( RENDERER_METAL ) || defined( RENDERER_D3D12 )
#include "LightTiler.hpp"
#endif

using namespace ae3d;
extern Renderer renderer;
float GetVRFov();

namespace MathUtil
{
    void GetMinMax( const std::vector< Vec3 >& aPoints, Vec3& outMin, Vec3& outMax );
    bool IsNaN( float f );
}

namespace Global
{
    extern Vec3 vrEyePosition;
}

#if defined( RENDERER_METAL ) || defined( RENDERER_D3D12 )
namespace GfxDeviceGlobal
{
    extern ae3d::LightTiler lightTiler;
}
#endif

namespace SceneGlobal
{
    GameObject shadowCamera;
    bool isShadowCameraCreated = false;
    Matrix44 shadowCameraViewMatrix;
    Matrix44 shadowCameraProjectionMatrix;
}

void SetupCameraForSpotShadowCasting( const Vec3& lightPosition, const Vec3& lightDirection, ae3d::CameraComponent& outCamera,
                                     ae3d::TransformComponent& outCameraTransform )
{
    outCameraTransform.LookAt( lightPosition, lightPosition + lightDirection * 200, Vec3( 0, 1, 0 ) );
    outCamera.SetProjectionType( ae3d::CameraComponent::ProjectionType::Perspective );
    outCamera.SetProjection( 45, 1, 1, 200 );
}

void SetupCameraForDirectionalShadowCasting( const Vec3& lightDirection, const Frustum& eyeFrustum, const Vec3& sceneAABBmin, const Vec3& sceneAABBmax,
                                             ae3d::CameraComponent& outCamera, ae3d::TransformComponent& outCameraTransform )
{
    const Vec3 viewFrustumCentroid = eyeFrustum.Centroid();

    System::Assert( !MathUtil::IsNaN( lightDirection.x ) && !MathUtil::IsNaN( lightDirection.y ) && !MathUtil::IsNaN( lightDirection.z ), "Invalid light direction" );
    System::Assert( !MathUtil::IsNaN( sceneAABBmin.x ) && !MathUtil::IsNaN( sceneAABBmin.y ) && !MathUtil::IsNaN( sceneAABBmin.z ), "Invalid scene AABB min" );
    System::Assert( !MathUtil::IsNaN( sceneAABBmax.x ) && !MathUtil::IsNaN( sceneAABBmax.y ) && !MathUtil::IsNaN( sceneAABBmax.z ), "Invalid scene AABB max" );
    System::Assert( !MathUtil::IsNaN( viewFrustumCentroid.x ) && !MathUtil::IsNaN( viewFrustumCentroid.y ) && !MathUtil::IsNaN( viewFrustumCentroid.z ), "Invalid eye frustum" );
    System::Assert( outCamera.GetTargetTexture() != nullptr, "Shadow camera needs target texture" );
    System::Assert( lightDirection.Length() > 0.9f && lightDirection.Length() < 1.1f, "Light dir must be normalized" );
    

    // Start at the centroid, and move back in the opposite direction of the light
    // by an amount equal to the camera's farClip. This is the temporary working position for the light.
    // TODO: Verify math. + was - in my last engine but had to be changed to get the right direction [TimoW, 2015-09-26]
    const Vec3 shadowCameraPosition = viewFrustumCentroid + lightDirection * eyeFrustum.FarClipPlane();
    
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

    if (nextFreeGameObject >= gameObjects.size())
    {
        gameObjects.resize( gameObjects.size() + 10 );
    }

    gameObjects[ nextFreeGameObject++ ] = gameObject;
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

void ae3d::Scene::RenderDepthAndNormalsForAllCameras( std::vector< GameObject* >& cameras )
{
    Statistics::BeginDepthNormalsProfiling();

    for (auto camera : cameras)
    {
        CameraComponent* cameraComponent = camera->GetComponent< CameraComponent >();

        if (cameraComponent->GetDepthNormalsTexture().GetID() != 0)
        {
            std::vector< unsigned > gameObjectsWithMeshRenderer;
            gameObjectsWithMeshRenderer.reserve( gameObjects.size() );
            int i = -1;

            for (auto gameObject : gameObjects)
            {
                ++i;

                if (gameObject == nullptr || (gameObject->GetLayer() & cameraComponent->GetLayerMask()) == 0 || !gameObject->IsEnabled())
                {
                    continue;
                }

                auto meshRenderer = gameObject->GetComponent< MeshRendererComponent >();

                if (meshRenderer)
                {
                    gameObjectsWithMeshRenderer.push_back( i );
                }
            }

            Frustum frustum;

            if (cameraComponent->GetProjectionType() == CameraComponent::ProjectionType::Perspective)
            {
                frustum.SetProjection( cameraComponent->GetFovDegrees(), cameraComponent->GetAspect(), cameraComponent->GetNear(), cameraComponent->GetFar() );
            }
            else
            {
                frustum.SetProjection( cameraComponent->GetLeft(), cameraComponent->GetRight(), cameraComponent->GetBottom(),
                                       cameraComponent->GetTop(), cameraComponent->GetNear(), cameraComponent->GetFar() );
            }

            auto cameraTransform = camera->GetComponent< TransformComponent >();
            // TODO: world position
            Vec3 position = cameraTransform ? cameraTransform->GetLocalPosition() : Vec3( 0, 0, 0 );
            const Matrix44& view = cameraComponent->GetView();
            const Vec3 viewDir = Vec3( view.m[ 2 ], view.m[ 6 ], view.m[ 10 ] ).Normalized();
            frustum.Update( position, viewDir );

            RenderDepthAndNormals( cameraComponent, view, gameObjectsWithMeshRenderer, 0, frustum );

#if defined( RENDERER_METAL ) || defined( RENDERER_D3D12 )
            int goWithPointLightIndex = 0;

            for (auto gameObject : gameObjects)
            {
                if (gameObject == nullptr || (gameObject->GetLayer() & cameraComponent->GetLayerMask()) == 0 || !gameObject->IsEnabled())
                {
                    continue;
                }

                auto transform = gameObject->GetComponent< TransformComponent >();
                auto pointLight = gameObject->GetComponent< PointLightComponent >();

                if (transform && pointLight)
                {
                    auto worldPos = transform->GetWorldPosition();
                    GfxDeviceGlobal::lightTiler.SetPointLightPositionAndRadius( goWithPointLightIndex, worldPos, pointLight->GetRadius());
                    ++goWithPointLightIndex;
                }
            }

            GfxDeviceGlobal::lightTiler.UpdateLightBuffers();
            GfxDeviceGlobal::lightTiler.CullLights( renderer.builtinShaders.lightCullShader, cameraComponent->GetProjection(),
                                                    view, cameraComponent->GetDepthNormalsTexture() );
#endif
        }
    }

    Statistics::EndDepthNormalsProfiling();
}

void ae3d::Scene::Render()
{
#if RENDERER_OPENGL
    Statistics::BeginFrameTimeProfiling();
#endif
#if RENDERER_VULKAN
    GfxDevice::BeginFrame();
#endif
    GenerateAABB();
#if RENDERER_D3D12
    GfxDevice::ResetCommandList();
#endif
    Statistics::ResetFrameStatistics();
    TransformComponent::UpdateLocalMatrices();

    std::vector< GameObject* > rtCameras;
    rtCameras.reserve( gameObjects.size() / 4 );
    std::vector< GameObject* > cameras;
    cameras.reserve( gameObjects.size() / 4 );
    
    for (auto gameObject : gameObjects)
    {
        if (gameObject == nullptr || !gameObject->IsEnabled())
        {
            continue;
        }

        auto cameraComponent = gameObject->GetComponent<CameraComponent>();
        
        if (cameraComponent && cameraComponent->GetTargetTexture() != nullptr && gameObject->GetComponent<TransformComponent>())
        {
            rtCameras.push_back( gameObject );
        }
        if (cameraComponent && cameraComponent->GetTargetTexture() == nullptr && gameObject->GetComponent<TransformComponent>())
        {
            cameras.push_back( gameObject );
        }
    }
#if RENDERER_VULKAN
    if (cameras.empty())
    {
        GfxDevice::BeginRenderPassAndCommandBuffer();
        GfxDevice::EndRenderPassAndCommandBuffer();
    }
#endif

    auto cameraSortFunction = [](GameObject* g1, GameObject* g2) { return g1->GetComponent< CameraComponent >()->GetRenderOrder() <
        g2->GetComponent< CameraComponent >()->GetRenderOrder(); };
    std::sort( std::begin( cameras ), std::end( cameras ), cameraSortFunction );
    
    for (auto rtCamera : rtCameras)
    {
        auto transform = rtCamera->GetComponent< TransformComponent >();

        if (transform && !rtCamera->GetComponent< CameraComponent >()->GetTargetTexture()->IsCube())
        {
            RenderWithCamera( rtCamera, 0, "2D RT" );
        }
        else if (transform && rtCamera->GetComponent< CameraComponent >()->GetTargetTexture()->IsCube())
        {
            const Vec3 cameraPos = transform->GetLocalPosition();

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
                    Vec3( 0,  -1,  0 ),
                    Vec3( 0,  -1,  0 ),
                    Vec3( 0,  0,  1 ),
                    Vec3( 0,  0, -1 ),
                    Vec3( 0, -1,  0 ),
                    Vec3( 0, -1,  0 )
                };
                
                transform->LookAt( cameraPos, cameraPos + directions[ cubeMapFace ], ups[ cubeMapFace ] );
                RenderWithCamera( rtCamera, cubeMapFace, "Cube Map RT" );
            }
        }
    }

    //unsigned debugShadowFBO = 0;
#if RENDERER_VULKAN
    GfxDevice::BeginRenderPassAndCommandBuffer();
#endif

    for (auto camera : cameras)
    {
        bool hasShadow = false;

        if (camera != nullptr && camera->GetComponent<TransformComponent>())
        {
            TransformComponent* cameraTransform = camera->GetComponent<TransformComponent>();

            auto cameraPos = cameraTransform->GetLocalPosition();
            auto cameraDir = cameraTransform->GetViewDirection();

            if (camera->GetComponent<CameraComponent>()->GetProjectionType() == ae3d::CameraComponent::ProjectionType::Perspective)
            {
                AudioSystem::SetListenerPosition( cameraPos.x, cameraPos.y, cameraPos.z );
                AudioSystem::SetListenerOrientation( cameraDir.x, cameraDir.y, cameraDir.z );
            }

            // Shadow pass
            Material::SetGlobalInt( "_LightType", 0 );

            for (auto go : gameObjects)
            {
                if (!go || !go->IsEnabled())
                {
                    continue;
                }
                
                auto lightTransform = go->GetComponent<TransformComponent>();
                auto dirLight = go->GetComponent<DirectionalLightComponent>();
                auto spotLight = go->GetComponent<SpotLightComponent>();
                auto pointLight = go->GetComponent<PointLightComponent>();

                if (lightTransform && ((dirLight && dirLight->CastsShadow()) || (spotLight && spotLight->CastsShadow()) ||
                                       (pointLight && pointLight->CastsShadow())))
                {
                    Statistics::BeginShadowMapProfiling();

                    Frustum eyeFrustum;
                    
                    auto cameraComponent = camera->GetComponent< CameraComponent >();
                    
                    if (cameraComponent->GetProjectionType() == CameraComponent::ProjectionType::Perspective)
                    {
                        eyeFrustum.SetProjection( cameraComponent->GetFovDegrees(), cameraComponent->GetAspect(), cameraComponent->GetNear(), cameraComponent->GetFar() );
                    }
                    else
                    {
                        eyeFrustum.SetProjection( cameraComponent->GetLeft(), cameraComponent->GetRight(), cameraComponent->GetBottom(), cameraComponent->GetTop(), cameraComponent->GetNear(), cameraComponent->GetFar() );
                    }
                    
                    Matrix44 eyeView;
                    cameraTransform->GetLocalRotation().GetMatrix( eyeView );
                    Matrix44 translation;
                    translation.Translate( -cameraTransform->GetLocalPosition() );
                    Matrix44::Multiply( translation, eyeView, eyeView );
                    
                    const Vec3 eyeViewDir = Vec3( eyeView.m[2], eyeView.m[6], eyeView.m[10] ).Normalized();
                    eyeFrustum.Update( cameraTransform->GetLocalPosition(), eyeViewDir );
                    
                    if (!SceneGlobal::isShadowCameraCreated)
                    {
                        SceneGlobal::shadowCamera.AddComponent< CameraComponent >();
                        SceneGlobal::shadowCamera.GetComponent< CameraComponent >()->SetClearFlag( ae3d::CameraComponent::ClearFlag::DepthAndColor );
                        SceneGlobal::shadowCamera.AddComponent< TransformComponent >();
                        SceneGlobal::isShadowCameraCreated = true;
                        // Component adding can invalidate lightTransform pointer.
                        lightTransform = go->GetComponent<TransformComponent>();
                    }
                    
                    if (dirLight)
                    {
                        SceneGlobal::shadowCamera.GetComponent< CameraComponent >()->SetTargetTexture( &go->GetComponent<DirectionalLightComponent>()->shadowMap );
                        SetupCameraForDirectionalShadowCasting( lightTransform->GetViewDirection(), eyeFrustum, aabbMin, aabbMax, *SceneGlobal::shadowCamera.GetComponent< CameraComponent >(), *SceneGlobal::shadowCamera.GetComponent< TransformComponent >() );
                    }
                    else if (spotLight)
                    {
                        SceneGlobal::shadowCamera.GetComponent< CameraComponent >()->SetTargetTexture( &go->GetComponent<SpotLightComponent>()->shadowMap );
                        SetupCameraForSpotShadowCasting( lightTransform->GetLocalPosition(), lightTransform->GetViewDirection(), *SceneGlobal::shadowCamera.GetComponent< CameraComponent >(), *SceneGlobal::shadowCamera.GetComponent< TransformComponent >() );
                    }
                    else if (pointLight)
                    {
                        SceneGlobal::shadowCamera.GetComponent< CameraComponent >()->SetTargetTexture( &go->GetComponent<PointLightComponent>()->shadowMap );
                    }

                    if (pointLight)
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
                                Vec3( 0,  -1,  0 ),
                                Vec3( 0,  -1,  0 ),
                                Vec3( 0,  0,  1 ),
                                Vec3( 0,  0, -1 ),
                                Vec3( 0, -1,  0 ),
                                Vec3( 0, -1,  0 )
                            };
                            
                            lightTransform->LookAt( lightTransform->GetLocalPosition(), lightTransform->GetLocalPosition() + directions[ cubeMapFace ], ups[ cubeMapFace ] );
                            RenderShadowsWithCamera( &SceneGlobal::shadowCamera, cubeMapFace );
                        }
                    }
                    else
                    {
                        RenderShadowsWithCamera( &SceneGlobal::shadowCamera, 0 );
                    }

                    if (dirLight)
                    {
                        Material::SetGlobalRenderTexture( "_ShadowMap", &go->GetComponent<DirectionalLightComponent>()->shadowMap );
                        Material::SetGlobalFloat( "_ShadowMinAmbient", 0.2f );
                        //debugShadowFBO = go->GetComponent<DirectionalLightComponent>()->shadowMap.GetFBO();
                        hasShadow = true;
                    }
                    else if (spotLight)
                    {
                        Material::SetGlobalRenderTexture( "_ShadowMap", &go->GetComponent<SpotLightComponent>()->shadowMap );
                        Material::SetGlobalFloat( "_ShadowMinAmbient", 0.2f );
#if RENDERER_OPENGL
                        Material::SetGlobalFloat( "_LightConeAngleCos", std::cos( spotLight->GetConeAngle() * 3.14159265f / 180.0f ) );
                        Material::SetGlobalVector( "_LightPosition", lightTransform->GetLocalPosition() );
                        Material::SetGlobalVector( "_LightDirection", lightTransform->GetViewDirection() );
                        Material::SetGlobalVector( "_LightColor", spotLight->GetColor() );
                        Material::SetGlobalInt( "_LightType", 1 );
#endif
                        //debugShadowFBO = go->GetComponent<SpotLightComponent>()->shadowMap.GetFBO();
                        hasShadow = true;
                    }
                    else if (pointLight)
                    {
                        Material::SetGlobalRenderTexture( "_ShadowMapCube", &go->GetComponent<PointLightComponent>()->shadowMap );
                        Material::SetGlobalFloat( "_ShadowMinAmbient", 0.2f );
                        //debugShadowFBO = go->GetComponent<SpotLightComponent>()->shadowMap.GetFBO();
                        hasShadow = true;
                    }

                    Statistics::EndShadowMapProfiling();
                }
            }
        }

        // Defaults for a case where there are no shadow casting lights.
#if !RENDERER_VULKAN
        if (!hasShadow)
        {
            Texture2D& whiteTexture = renderer.GetWhiteTexture();
            Material::SetGlobalTexture2D( "_ShadowMap", &whiteTexture );
            Material::SetGlobalFloat( "_ShadowMinAmbient", 1 );
        }
#endif
#if !RENDERER_METAL
        RenderWithCamera( camera, 0, "Primary Pass" );
#endif
    }
#if RENDERER_VULKAN
    GfxDevice::EndRenderPassAndCommandBuffer();
#endif

    //GfxDevice::DebugBlitFBO( debugShadowFBO, 256, 256 );

    RenderDepthAndNormalsForAllCameras( cameras );

#if RENDERER_METAL
    GfxDevice::BeginBackBufferEncoding();

    for (auto camera : cameras)
    {
        RenderWithCamera( camera, 0, "Primary Pass" );
    }

    GfxDevice::EndBackBufferEncoding();
    Statistics::EndFrameTimeProfiling();
#endif
}

void ae3d::Scene::RenderWithCamera( GameObject* cameraGo, int cubeMapFace, const char* debugGroupName )
{
    ae3d::System::Assert( 0 <= cubeMapFace && cubeMapFace < 6, "invalid cube map face" );

    CameraComponent* camera = cameraGo->GetComponent< CameraComponent >();
    const Vec3 color = camera->GetClearColor();
    GfxDevice::SetClearColor( color.x, color.y, color.z );
#ifndef RENDERER_METAL
    GfxDevice::SetRenderTarget( camera->GetTargetTexture(), cubeMapFace );
#endif
#ifndef RENDERER_METAL
    GfxDevice::PushGroupMarker( debugGroupName );
#endif
    if (camera->GetClearFlag() == CameraComponent::ClearFlag::DepthAndColor)
    {
        GfxDevice::ClearScreen( GfxDevice::ClearFlags::Color | GfxDevice::ClearFlags::Depth );
    }
    else if (camera->GetClearFlag() == CameraComponent::ClearFlag::Depth)
    {
        GfxDevice::ClearScreen( GfxDevice::ClearFlags::Depth );
    }
    else if (camera->GetClearFlag() == CameraComponent::ClearFlag::DontClear)
    {
        GfxDevice::ClearScreen( GfxDevice::ClearFlags::DontClear );
    }
    else
    {
        System::Assert( false, "Unhandled clear flag." );
    }
#if RENDERER_METAL
    GfxDevice::SetRenderTarget( camera->GetTargetTexture(), cubeMapFace );
    GfxDevice::PushGroupMarker( debugGroupName );
#endif
    
    Matrix44 view;

    if (skybox != nullptr && camera->GetProjectionType() != ae3d::CameraComponent::ProjectionType::Orthographic)
    {
        auto cameraTrans = cameraGo->GetComponent< TransformComponent >();
        cameraTrans->GetLocalRotation().GetMatrix( view );
#if defined( OCULUS_RIFT ) || defined( AE3D_OPENVR )
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
    
    float fovDegrees;
    Vec3 position;

    // TODO: Maybe add a VR flag into camera to select between HMD and normal pose.
#if defined( OCULUS_RIFT ) || defined( AE3D_OPENVR )
    view = cameraGo->GetComponent< TransformComponent >()->GetVrView();
    position = Global::vrEyePosition;
    fovDegrees = GetVRFov();
#else
    auto cameraTransform = cameraGo->GetComponent< TransformComponent >();
    position = cameraTransform->GetLocalPosition();
    fovDegrees = camera->GetFovDegrees();
    cameraTransform->GetLocalRotation().GetMatrix( view );
    Matrix44 translation;
    translation.Translate( -cameraTransform->GetLocalPosition() );
    Matrix44::Multiply( translation, view, view );
    camera->SetView( view );
#endif
    
    Frustum frustum;
    
    if (camera->GetProjectionType() == CameraComponent::ProjectionType::Perspective)
    {
        frustum.SetProjection( fovDegrees, camera->GetAspect(), camera->GetNear(), camera->GetFar() );
    }
    else
    {
        frustum.SetProjection( camera->GetLeft(), camera->GetRight(), camera->GetBottom(), camera->GetTop(), camera->GetNear(), camera->GetFar() );
    }
    
    const Vec3 viewDir = Vec3( view.m[2], view.m[6], view.m[10] ).Normalized();
    frustum.Update( position, viewDir );

    std::vector< unsigned > gameObjectsWithMeshRenderer;
    gameObjectsWithMeshRenderer.reserve( gameObjects.size() );
    int i = -1;
    
    for (auto gameObject : gameObjects)
    {
        ++i;
        
        if (gameObject == nullptr || (gameObject->GetLayer() & camera->GetLayerMask()) == 0 || !gameObject->IsEnabled())
        {
            continue;
        }
        
        auto transform = gameObject->GetComponent< TransformComponent >();
        auto spriteRenderer = gameObject->GetComponent< SpriteRendererComponent >();

        if (spriteRenderer)
        {
            Matrix44 projectionModel;
            Matrix44::Multiply( transform ? transform->GetLocalToWorldMatrix() : Matrix44::identity, camera->GetProjection(), projectionModel );
            spriteRenderer->Render( projectionModel.m );
        }
        
        auto textRenderer = gameObject->GetComponent< TextRendererComponent >();
        
        if (textRenderer)
        {
            Matrix44 projectionModel;
            Matrix44::Multiply( transform ? transform->GetLocalToWorldMatrix() : Matrix44::identity, camera->GetProjection(), projectionModel );
            textRenderer->Render( projectionModel.m );
        }
        
        auto meshRenderer = gameObject->GetComponent< MeshRendererComponent >();

        if (meshRenderer)
        {
            gameObjectsWithMeshRenderer.push_back( i );
        }
    }

    auto meshSorterByMesh = [&](unsigned j, unsigned k)
    {
        return gameObjects[ j ]->GetComponent< MeshRendererComponent >()->GetMesh() <
               gameObjects[ k ]->GetComponent< MeshRendererComponent >()->GetMesh();
    };

    std::sort( std::begin( gameObjectsWithMeshRenderer ), std::end( gameObjectsWithMeshRenderer ), meshSorterByMesh );
    
    for (auto j : gameObjectsWithMeshRenderer)
    {
        auto transform = gameObjects[ j ]->GetComponent< TransformComponent >();
        auto meshLocalToWorld = transform ? transform->GetLocalToWorldMatrix() : Matrix44::identity;

        Matrix44 mv;
        Matrix44 mvp;
        Matrix44::Multiply( meshLocalToWorld, view, mv );
        Matrix44::Multiply( mv, camera->GetProjection(), mvp );

        auto* meshRenderer = gameObjects[ j ]->GetComponent< MeshRendererComponent >();
        meshRenderer->Cull( frustum, meshLocalToWorld );
        meshRenderer->Render( mv, mvp, meshLocalToWorld, SceneGlobal::shadowCameraViewMatrix, SceneGlobal::shadowCameraProjectionMatrix, nullptr, MeshRendererComponent::RenderType::Opaque );
    }

    for (auto j : gameObjectsWithMeshRenderer)
    {
        auto transform = gameObjects[ j ]->GetComponent< TransformComponent >();
        auto meshLocalToWorld = transform ? transform->GetLocalToWorldMatrix() : Matrix44::identity;
        
        Matrix44 mv;
        Matrix44 mvp;
        Matrix44::Multiply( meshLocalToWorld, view, mv );
        Matrix44::Multiply( mv, camera->GetProjection(), mvp );
        
        gameObjects[ j ]->GetComponent< MeshRendererComponent >()->Render( mv, mvp, meshLocalToWorld, SceneGlobal::shadowCameraViewMatrix, SceneGlobal::shadowCameraProjectionMatrix, nullptr, MeshRendererComponent::RenderType::Transparent );
    }

    GfxDevice::PopGroupMarker();

#if RENDERER_METAL
    GfxDevice::UnsetRenderTarget();
#endif
    GfxDevice::ErrorCheck( "Scene render after rendering" );
}

void ae3d::Scene::RenderDepthAndNormals( CameraComponent* camera, const Matrix44& view, std::vector< unsigned > gameObjectsWithMeshRenderer,
                                         int cubeMapFace, const Frustum& frustum )
{
#if RENDERER_METAL
    GfxDevice::ClearScreen( GfxDevice::ClearFlags::Color | GfxDevice::ClearFlags::Depth );
    GfxDevice::SetRenderTarget( &camera->GetDepthNormalsTexture(), cubeMapFace );
#else
    GfxDevice::SetRenderTarget( &camera->GetDepthNormalsTexture(), cubeMapFace );
    GfxDevice::ClearScreen( GfxDevice::ClearFlags::Color | GfxDevice::ClearFlags::Depth );
#endif
    GfxDevice::PushGroupMarker( "DepthNormal" );

    for (auto j : gameObjectsWithMeshRenderer)
    {
        auto transform = gameObjects[ j ]->GetComponent< TransformComponent >();
        auto meshLocalToWorld = transform ? transform->GetLocalToWorldMatrix() : Matrix44::identity;
        
        Matrix44 mv;
        Matrix44 mvp;
        Matrix44::Multiply( meshLocalToWorld, view, mv );
        Matrix44::Multiply( mv, camera->GetProjection(), mvp );
        
        auto meshRenderer = gameObjects[ j ]->GetComponent< MeshRendererComponent >();

        meshRenderer->Cull( frustum, meshLocalToWorld );
        meshRenderer->Render( mv, mvp, meshLocalToWorld, SceneGlobal::shadowCameraViewMatrix, SceneGlobal::shadowCameraProjectionMatrix, &renderer.builtinShaders.depthNormalsShader, MeshRendererComponent::RenderType::Opaque );
        meshRenderer->Render( mv, mvp, meshLocalToWorld, SceneGlobal::shadowCameraViewMatrix, SceneGlobal::shadowCameraProjectionMatrix, &renderer.builtinShaders.depthNormalsShader, MeshRendererComponent::RenderType::Transparent );
    }

    GfxDevice::PopGroupMarker();

#if RENDERER_METAL
    GfxDevice::UnsetRenderTarget();
#else
    GfxDevice::SetRenderTarget( nullptr, 0 );
#endif
    GfxDevice::ErrorCheck( "depthnormals render end" );
}

void ae3d::Scene::RenderShadowsWithCamera( GameObject* cameraGo, int cubeMapFace )
{
    CameraComponent* camera = cameraGo->GetComponent< CameraComponent >();
#if !RENDERER_METAL
    GfxDevice::SetRenderTarget( camera->GetTargetTexture(), cubeMapFace );
#endif

    const Vec3 color = camera->GetClearColor();
    GfxDevice::SetClearColor( color.x, color.y, color.z );
    
    if (camera->GetClearFlag() == CameraComponent::ClearFlag::DepthAndColor)
    {
        GfxDevice::ClearScreen( GfxDevice::ClearFlags::Color | GfxDevice::ClearFlags::Depth );
    }
#if RENDERER_METAL
    GfxDevice::SetRenderTarget( camera->GetTargetTexture(), cubeMapFace );
#endif
#if RENDERER_VULKAN
	GfxDevice::BeginRenderPassAndCommandBuffer();
#endif
    GfxDevice::PushGroupMarker( "Shadow maps" );

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
    
    auto meshSorterByMesh = [&](unsigned j, unsigned k)
    {
        return gameObjects[ j ]->GetComponent< MeshRendererComponent >()->GetMesh() <
               gameObjects[ k ]->GetComponent< MeshRendererComponent >()->GetMesh();
    };
    std::sort( std::begin( gameObjectsWithMeshRenderer ), std::end( gameObjectsWithMeshRenderer ), meshSorterByMesh );
    
    for (auto j : gameObjectsWithMeshRenderer)
    {
        auto transform = gameObjects[ j ]->GetComponent< TransformComponent >();
        auto meshLocalToWorld = transform ? transform->GetLocalToWorldMatrix() : Matrix44::identity;
        
        Matrix44 mv;
        Matrix44 mvp;
        Matrix44::Multiply( meshLocalToWorld, view, mv );
        Matrix44::Multiply( mv, camera->GetProjection(), mvp );

        auto* meshRenderer = gameObjects[ j ]->GetComponent< MeshRendererComponent >();

        meshRenderer->Cull( frustum, meshLocalToWorld );
        meshRenderer->Render( mv, mvp, meshLocalToWorld, SceneGlobal::shadowCameraViewMatrix, SceneGlobal::shadowCameraProjectionMatrix, &renderer.builtinShaders.momentsShader, MeshRendererComponent::RenderType::Opaque );
    }

    GfxDevice::PopGroupMarker();

#if RENDERER_METAL
    GfxDevice::UnsetRenderTarget();
#endif
#if RENDERER_VULKAN
	GfxDevice::EndRenderPassAndCommandBuffer();
#endif

    GfxDevice::ErrorCheck( "Scene render shadows end" );
}

void ae3d::Scene::SetSkybox( TextureCube* skyTexture )
{
    skybox = skyTexture;
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
        if (gameObject->GetComponent<MeshRendererComponent>())
        {
            outSerialized += gameObject->GetComponent<MeshRendererComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<TransformComponent>())
        {
            outSerialized += gameObject->GetComponent<TransformComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<CameraComponent>())
        {
            outSerialized += gameObject->GetComponent<CameraComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<SpriteRendererComponent>())
        {
            outSerialized += gameObject->GetComponent<SpriteRendererComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<TextRendererComponent>())
        {
            outSerialized += gameObject->GetComponent<TextRendererComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<AudioSourceComponent>())
        {
            outSerialized += gameObject->GetComponent<AudioSourceComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<DirectionalLightComponent>())
        {
            outSerialized += gameObject->GetComponent<DirectionalLightComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<SpotLightComponent>())
        {
            outSerialized += gameObject->GetComponent<SpotLightComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<PointLightComponent>())
        {
            outSerialized += gameObject->GetComponent<PointLightComponent>()->GetSerialized();
        }
    }
    return outSerialized;
}

ae3d::Scene::DeserializeResult ae3d::Scene::Deserialize( const FileSystem::FileContentsData& serialized, std::vector< GameObject >& outGameObjects,
                                                        std::map< std::string, Texture2D* >& outTexture2Ds,
                                                        std::map< std::string, Material* >& outMaterials,
                                                        std::vector< Mesh* >& outMeshes ) const
{
    // TODO: It would be better to store the token strings into somewhere accessible to GetSerialized() to prevent typos etc.

    outGameObjects.clear();

    std::stringstream stream( std::string( std::begin( serialized.data ), std::end( serialized.data ) ) );
    std::string line;
    
    std::string currentMaterialName;
    
    enum CurrentLightType { Directional, Spot, Point, None };
    CurrentLightType currentLightType = CurrentLightType::None;
    
    // FIXME: These ensure that the mesh is rendered. A proper fix would be to serialize materials.
    static Shader tempShader;
    tempShader.Load( FileSystem::FileContents( "unlit.vsh" ), FileSystem::FileContents( "unlit.fsh" ),
        "unlit_vertex", "unlit_fragment",
        FileSystem::FileContents( "unlit.hlsl" ), FileSystem::FileContents( "unlit.hlsl" ),
        FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );

    Material* tempMaterial = new Material();
    tempMaterial->SetShader( &tempShader );
    tempMaterial->SetTexture( "textureMap", Texture2D::GetDefaultTexture() );
    tempMaterial->SetVector( "tint", { 1, 1, 1, 1 } );
    tempMaterial->SetBackFaceCulling( true );
    outMaterials[ "temp material" ] = tempMaterial;

    while (!stream.eof())
    {
        std::getline( stream, line );
        std::stringstream lineStream( line );
        std::locale c_locale( "C" );
        lineStream.imbue( c_locale );
        std::string token;
        lineStream >> token;
        
        if (token == "gameobject")
        {
            outGameObjects.push_back( GameObject() );
            std::string name;
            lineStream >> name;
            outGameObjects.back().SetName( name.c_str() );
        }
        else if (token == "name")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found name but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            std::string name;
            std::getline( lineStream, name );
            outGameObjects.back().SetName( name.c_str() );
        }
        else if (token == "layer")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found layer but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            int layer;
            lineStream >> layer;
            outGameObjects.back().SetLayer( layer );
        }
        else if (token == "enabled")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found layer but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            int enabled;
            lineStream >> enabled;
            outGameObjects.back().SetEnabled( enabled != 0 );
        }
        else if (token == "dirlight")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found dirlight but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }
            
            currentLightType = CurrentLightType::Directional;
            outGameObjects.back().AddComponent< DirectionalLightComponent >();
            
            int castsShadow = 0;
            std::string shadowStr;
            lineStream >> shadowStr;
            lineStream >> castsShadow;
            outGameObjects.back().GetComponent< DirectionalLightComponent >()->SetCastShadow( castsShadow != 0, 512 );
        }
        else if (token == "spotlight")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found spotlight but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }
            
            currentLightType = CurrentLightType::Spot;
            GameObject& go = outGameObjects.back();
            go.AddComponent< SpotLightComponent >();            
        }
        else if (token == "pointlight")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found pointlight but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }
            
            currentLightType = CurrentLightType::Point;
            GameObject& go  = outGameObjects.back();
            go.AddComponent< PointLightComponent >();            
        }
        else if (token == "shadow")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found shadow but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            int enabled;
            lineStream >> enabled;

            if (currentLightType == CurrentLightType::Directional)
            {
                outGameObjects.back().GetComponent< DirectionalLightComponent >()->SetCastShadow( enabled != 0, 1024 );
            }
            else if (currentLightType == CurrentLightType::Spot)
            {
                outGameObjects.back().GetComponent< SpotLightComponent >()->SetCastShadow( enabled != 0, 1024 );
            }
            else if (currentLightType == CurrentLightType::Point)
            {
                outGameObjects.back().GetComponent< PointLightComponent >()->SetCastShadow( enabled != 0, 1024 );
            }
        }
        else if (token == "camera")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found camera but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            outGameObjects.back().AddComponent< CameraComponent >();
        }
        else if (token == "ortho")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found ortho but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            float x, y, width, height, nearp, farp;
            lineStream >> x >> y >> width >> height >> nearp >> farp;
            outGameObjects.back().GetComponent< CameraComponent >()->SetProjection( x, y, width, height, nearp, farp );
        }
        else if (token == "persp")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found persp but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            float fov, aspect, nearp, farp;
            lineStream >> fov >> aspect >> nearp >> farp;
            outGameObjects.back().GetComponent< CameraComponent >()->SetProjection( fov, aspect, nearp, farp );
        }
        else if (token == "projection")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found projection but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

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
            else
            {
                System::Print( "Camera has unknown projection type %s\n", type.c_str() );
                return DeserializeResult::ParseError;
            }
        }
        else if (token == "clearcolor")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found clearcolor but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            float red, green, blue;
            lineStream >> red >> green >> blue;
            outGameObjects.back().GetComponent< CameraComponent >()->SetClearColor( { red, green, blue } );
        }
        else if (token == "transform")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found transform but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            outGameObjects.back().AddComponent< TransformComponent >();
        }
        else if (token == "meshrenderer")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found meshrenderer but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            outGameObjects.back().AddComponent< MeshRendererComponent >();
            
            outMeshes.push_back( new Mesh() );
        }
        else if (token == "spriterenderer")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found spriterenderer but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            outGameObjects.back().AddComponent< SpriteRendererComponent >();
        }
        else if (token == "sprite")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found sprite but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            if (!outGameObjects.back().GetComponent< SpriteRendererComponent >())
            {
                System::Print( "Failed to parse %s: found sprite but the game object doesn't have a sprite renderer component.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            std::string spritePath;
            float x, y, width, height;
            lineStream >> spritePath >> x >> y >> width >> height;

            outTexture2Ds[ spritePath ] = new Texture2D();
            outTexture2Ds[ spritePath ]->Load( FileSystem::FileContents( spritePath.c_str() ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );

            outGameObjects.back().GetComponent< SpriteRendererComponent >()->SetTexture( outTexture2Ds[ spritePath ], Vec3( x, y, 0 ), Vec3( x, y, 1 ), Vec4( 1, 1, 1, 1 ) );
        }
        else if (token == "meshpath")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found meshFile but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            auto meshRenderer = outGameObjects.back().GetComponent< MeshRendererComponent >();

            if (!meshRenderer)
            {
                System::Print( "Failed to parse %s: found meshpath but the game object doesn't have a mesh renderer component.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            std::string meshFile;
            lineStream >> meshFile;

            outMeshes.back()->Load( FileSystem::FileContents( meshFile.c_str() ) );
			meshRenderer->SetMesh( outMeshes.back() );

            meshRenderer->SetMaterial( tempMaterial, 0 );
        }
        else if (token == "position")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found position but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            if (!outGameObjects.back().GetComponent< TransformComponent >())
            {
                System::Print( "Failed to parse %s: found position but the game object doesn't have a transform component.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            float x, y, z;
            lineStream >> x >> y >> z;
            outGameObjects.back().GetComponent< TransformComponent >()->SetLocalPosition( { x, y, z } );
        }
        else if (token == "rotation")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found rotation but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            if (!outGameObjects.back().GetComponent< TransformComponent >())
            {
                System::Print( "Failed to parse %s: found rotation but the game object doesn't have a transform component.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            float x, y, z, s;
            lineStream >> x >> y >> z >> s;
            outGameObjects.back().GetComponent< TransformComponent >()->SetLocalRotation( { { x, y, z }, s } );
        }
        else if (token == "scale")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found scale but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }
            
            if (!outGameObjects.back().GetComponent< TransformComponent >())
            {
                System::Print( "Failed to parse %s: found scale but the game object doesn't have a transform component.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            float scale;
            lineStream >> scale;
            outGameObjects.back().GetComponent< TransformComponent >()->SetLocalScale( scale );
        }
        else if (token == "texture2d")
        {
            std::string name;
            std::string path;
            
            lineStream >> name >> path;
            
            outTexture2Ds[ name ] = new Texture2D();
            outTexture2Ds[ name ]->Load( FileSystem::FileContents( path.c_str() ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );
        }
        else if (token == "material")
        {
            std::string materialName;
            lineStream >> materialName;
            currentMaterialName = materialName;
            
            outMaterials[ materialName ] = new Material();
        }
        else if (token == "mesh_material")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found mesh_material but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }
            
            auto mr = outGameObjects.back().GetComponent< MeshRendererComponent >();

            if (!mr)
            {
                System::Print( "Failed to parse %s: found mesh_material but the last defined game object doesn't have a mesh renderer component.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }
            
            if (!mr->GetMesh())
            {
                System::Print( "Failed to parse %s: found mesh_material but the last defined game object's mesh renderer doesn't have a mesh.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }
            
            std::string meshName;
            std::string materialName;
            lineStream >> meshName >> materialName;
            
            const unsigned subMeshCount = mr->GetMesh()->GetSubMeshCount();
            
            for (unsigned i = 0; i < subMeshCount; ++i)
            {
                if (mr->GetMesh()->GetSubMeshName( i ) == meshName)
                {
                    mr->SetMaterial( outMaterials[ materialName ], i );
                }
            }
        }
        else if (token == "param_float")
        {
            std::string uniformName;
            float floatValue;

            lineStream >> uniformName >> floatValue;
            outMaterials[ currentMaterialName ]->SetFloat( uniformName.c_str(), floatValue );
        }
        else if (token == "param_vec3")
        {
            std::string uniformName;
            Vec3 vec3;

            lineStream >> uniformName >> vec3.x >> vec3.y >> vec3.z;
            outMaterials[ currentMaterialName ]->SetVector( uniformName.c_str(), vec3 );
        }
        else if (token == "param_vec4")
        {
            std::string uniformName;
            Vec4 vec4;

            lineStream >> uniformName >> vec4.x >> vec4.y >> vec4.z >> vec4.w;
            outMaterials[ currentMaterialName ]->SetVector( uniformName.c_str(), vec4 );
        }
        else if (token == "param_texture")
        {
            std::string uniformName;
            std::string textureName;
            
            lineStream >> uniformName >> textureName;
            outMaterials[ currentMaterialName ]->SetTexture( uniformName.c_str(), outTexture2Ds[ textureName ]);
        }
        else if (token == "audiosource")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s: found audiosource but there are no game objects defined before this line.\n", serialized.path.c_str() );
                return DeserializeResult::ParseError;
            }

            outGameObjects.back().AddComponent< AudioSourceComponent >();
        }
        else if (token == "coneangle")
        {
            float coneAngleDegrees;
            lineStream >> coneAngleDegrees;
            outGameObjects.back().GetComponent< SpotLightComponent >()->SetConeAngle( coneAngleDegrees );
        }
        else if (token == "radius")
        {
            float radius;
            lineStream >> radius;
            outGameObjects.back().GetComponent< PointLightComponent >()->SetRadius( radius );
        }
        else if (token == "color")
        {
            Vec3 color;

            lineStream >> color.x >> color.y >> color.z;

            if (currentLightType == CurrentLightType::Spot)
            {
                outGameObjects.back().GetComponent< SpotLightComponent >()->SetColor( color );
            }
            else if (currentLightType == CurrentLightType::Directional)
            {
                outGameObjects.back().GetComponent< DirectionalLightComponent >()->SetColor( color );
            }
            else if (currentLightType == CurrentLightType::Point)
            {
                outGameObjects.back().GetComponent< PointLightComponent >()->SetColor( color );
            }
        }
        else
        {
            System::Print( "Unhandled token %s\n", token.c_str() );
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
        
        Vec3 oAABBmin = meshRenderer->GetMesh() ? meshRenderer->GetMesh()->GetAABBMin() : Vec3( -1, -1, -1 );
        Vec3 oAABBmax = meshRenderer->GetMesh() ? meshRenderer->GetMesh()->GetAABBMax() : Vec3(  1,  1,  1 );
        
        Matrix44::TransformPoint( oAABBmin, meshTransform->GetLocalToWorldMatrix(), &oAABBmin );
        Matrix44::TransformPoint( oAABBmax, meshTransform->GetLocalToWorldMatrix(), &oAABBmax );
        
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
