#include "Scene.hpp"
#include <algorithm>
#include <locale>
#include <string>
#include <sstream>
#include <vector>
#include "AudioSourceComponent.hpp"
#include "AudioSystem.hpp"
#include "CameraComponent.hpp"
#include "DirectionalLightComponent.hpp"
#include "FileSystem.hpp"
#include "Frustum.hpp"
#include "GameObject.hpp"
#include "GfxDevice.hpp"
#include "LightTiler.hpp"
#include "Matrix.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "MeshRendererComponent.hpp"
#include "PointLightComponent.hpp"
#include "RenderTexture.hpp"
#include "Renderer.hpp"
#include "SpriteRendererComponent.hpp"
#include "SpotLightComponent.hpp"
#include "Statistics.hpp"
#include "System.hpp"
#include "TextRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "Texture2D.hpp"

using namespace ae3d;
extern Renderer renderer;
float GetVRFov();
void BeginOffscreen();
void EndOffscreen();
std::string GetSerialized( ae3d::TextRendererComponent* component );
std::string GetSerialized( ae3d::CameraComponent* component );
std::string GetSerialized( ae3d::AudioSourceComponent* component );
std::string GetSerialized( ae3d::MeshRendererComponent* component );
std::string GetSerialized( ae3d::DirectionalLightComponent* component );
std::string GetSerialized( ae3d::PointLightComponent* component );
std::string GetSerialized( ae3d::SpotLightComponent* component );
std::string GetSerialized( ae3d::TransformComponent* component );

namespace GfxDeviceGlobal
{
    extern PerObjectUboStruct perObjectUboStruct;
    extern ae3d::LightTiler lightTiler;
}

namespace MathUtil
{
    void GetMinMax( const Vec3* aPoints, int count, Vec3& outMin, Vec3& outMax );
    bool IsNaN( float f );
}

namespace Global
{
    extern Vec3 vrEyePosition;
}

namespace VRGlobal
{
    extern int eye;
}

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
#ifdef RENDERER_METAL
    outCameraTransform.LookAt( lightPosition, lightPosition - lightDirection * 200, Vec3( 0, 1, 0 ) );
#else
    outCameraTransform.LookAt( lightPosition, lightPosition + lightDirection * 200, Vec3( 0, 1, 0 ) );
#endif
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
    translation.SetTranslation( -outCameraTransform.GetLocalPosition() );
    Matrix44::Multiply( translation, view, view );
    outCamera.SetView( view );

    const Matrix44& shadowCameraView = outCamera.GetView();
    
    // Transforms view camera frustum points to shadow camera space.
    Vec3 viewFrustumLS[ 8 ];
    
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
    MathUtil::GetMinMax( viewFrustumLS, 8, viewMinLS, viewMaxLS );
    
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
    
    Vec3 sceneAABBLS[ 8 ];
    
    for (int i = 0; i < 8; ++i)
    {
        Matrix44::TransformPoint( sceneCorners[ i ], shadowCameraView, &sceneAABBLS[ i ] );
    }
    
    MathUtil::GetMinMax( sceneAABBLS, 8, sceneAABBminLS, sceneAABBmaxLS );
    
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

            int goWithPointLightIndex = 0;
            int goWithSpotLightIndex = 0;
            
            for (auto gameObject : gameObjects)
            {
                if (gameObject == nullptr || (gameObject->GetLayer() & cameraComponent->GetLayerMask()) == 0 || !gameObject->IsEnabled())
                {
                    continue;
                }

                auto transform = gameObject->GetComponent< TransformComponent >();
                auto pointLight = gameObject->GetComponent< PointLightComponent >();
                auto spotLight = gameObject->GetComponent< SpotLightComponent >();

                if (transform && pointLight)
                {
                    auto worldPos = transform->GetWorldPosition();
                    GfxDeviceGlobal::lightTiler.SetPointLightParameters( goWithPointLightIndex, worldPos, pointLight->GetRadius(), Vec4( pointLight->GetColor() ) );
                    ++goWithPointLightIndex;
                }

                if (transform && spotLight)
                {
                    auto worldPos = transform->GetWorldPosition();
                    // FIXME: add falloff radius, check cone angle vs. cosine of cone angle
                    GfxDeviceGlobal::lightTiler.SetSpotLightParameters( goWithSpotLightIndex, worldPos, spotLight->GetRadius(), Vec4( spotLight->GetColor() ), transform->GetViewDirection(), spotLight->GetConeAngle(), 3 );
                    ++goWithSpotLightIndex;
                }
            }

            GfxDeviceGlobal::lightTiler.UpdateLightBuffers();
            Statistics::BeginLightCullerProfiling();
            GfxDeviceGlobal::lightTiler.CullLights( renderer.builtinShaders.lightCullShader, cameraComponent->GetProjection(),
                                                    view, cameraComponent->GetDepthNormalsTexture() );
            Statistics::EndLightCullerProfiling();
        }
    }

    Statistics::EndDepthNormalsProfiling();
}

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

void ae3d::Scene::SetAmbient( const Vec3& color )
{
    ambientColor = color;
}

void ae3d::Scene::RenderRTCameras( std::vector< GameObject* >& rtCameras )
{
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
                transform->LookAt( cameraPos, cameraPos + directions[ cubeMapFace ], ups[ cubeMapFace ] );
                RenderWithCamera( rtCamera, cubeMapFace, "Cube Map RT" );
            }
        }
    }
}

void ae3d::Scene::RenderShadowMaps( std::vector< GameObject* >& cameras )
{
    for (auto camera : cameras)
    {
        if (camera != nullptr && camera->GetComponent<TransformComponent>())
        {
            TransformComponent* cameraTransform = camera->GetComponent<TransformComponent>();

            // Shadow pass

            if (camera->GetComponent<CameraComponent>()->GetProjectionType() == ae3d::CameraComponent::ProjectionType::Perspective)
            {
                auto cameraPos = cameraTransform->GetWorldPosition();
                auto cameraDir = cameraTransform->GetViewDirection();

                AudioSystem::SetListenerPosition( cameraPos.x, cameraPos.y, cameraPos.z );
                AudioSystem::SetListenerOrientation( cameraDir.x, cameraDir.y, cameraDir.z );

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
                        cameraTransform->GetWorldRotation().GetMatrix( eyeView );
                        Matrix44 translation;
                        translation.SetTranslation( -cameraTransform->GetWorldPosition() );
                        Matrix44::Multiply( translation, eyeView, eyeView );
                    
                        const Vec3 eyeViewDir = Vec3( eyeView.m[2], eyeView.m[6], eyeView.m[10] ).Normalized();
                        eyeFrustum.Update( cameraTransform->GetWorldPosition(), eyeViewDir );
                    
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
                            GfxDeviceGlobal::perObjectUboStruct.lightType = 2;
                            RenderShadowsWithCamera( &SceneGlobal::shadowCamera, 0 );
                            Material::SetGlobalRenderTexture( "_ShadowMap", &go->GetComponent<DirectionalLightComponent>()->shadowMap );
                        }
                        else if (spotLight)
                        {
                            SceneGlobal::shadowCamera.GetComponent< CameraComponent >()->SetTargetTexture( &go->GetComponent<SpotLightComponent>()->shadowMap );
                            SetupCameraForSpotShadowCasting( lightTransform->GetWorldPosition(), lightTransform->GetViewDirection(), *SceneGlobal::shadowCamera.GetComponent< CameraComponent >(), *SceneGlobal::shadowCamera.GetComponent< TransformComponent >() );
                            GfxDeviceGlobal::perObjectUboStruct.lightType = 1;
                            RenderShadowsWithCamera( &SceneGlobal::shadowCamera, 0 );
                            Material::SetGlobalRenderTexture( "_ShadowMap", &go->GetComponent<SpotLightComponent>()->shadowMap );
                        }
                        else if (pointLight)
                        {
                            SceneGlobal::shadowCamera.GetComponent< CameraComponent >()->SetTargetTexture( &go->GetComponent<PointLightComponent>()->shadowMap );
                            GfxDeviceGlobal::perObjectUboStruct.lightType = 3;
                            
                            for (int cubeMapFace = 0; cubeMapFace < 6; ++cubeMapFace)
                            {
                                lightTransform->LookAt( lightTransform->GetLocalPosition(), lightTransform->GetLocalPosition() + directions[ cubeMapFace ], ups[ cubeMapFace ] );
                                RenderShadowsWithCamera( &SceneGlobal::shadowCamera, cubeMapFace );
                            }

                            Material::SetGlobalRenderTexture( "_ShadowMapCube", &go->GetComponent<PointLightComponent>()->shadowMap );
                        }

                        Statistics::EndShadowMapProfiling();
                    }
                }
            }
        }
    }
}

void BubbleSort( GameObject** gos, int count )
{
    for (int i = 0; i < count - 1; ++i)
    {
        for (int j = 0; j < count - i - 1; ++j)
        {
            if (gos[ j ]->GetComponent< CameraComponent >()->GetRenderOrder() >
                gos[ j + 1 ]->GetComponent< CameraComponent >()->GetRenderOrder())
            {
                GameObject* temp = gos[ j ];
                gos[ j ] = gos[ j + 1 ];
                gos[ j + 1 ] = temp;
            }
        }
    }
}

void ae3d::Scene::Render()
{
#if RENDERER_VULKAN
#if AE3D_OPENVR
    if (VRGlobal::eye == 0)
    {
        GfxDevice::BeginFrame();
    }
#else
    GfxDevice::BeginFrame();
#endif

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
    //BubbleSort( cameras.data(), (int)cameras.size() );
    
    RenderShadowMaps( rtCameras );
    RenderDepthAndNormalsForAllCameras( rtCameras );
    RenderRTCameras( rtCameras );
    RenderDepthAndNormalsForAllCameras( cameras );

#if RENDERER_VULKAN
    GfxDevice::BeginRenderPassAndCommandBuffer();
#endif

#if RENDERER_METAL
    GfxDevice::BeginBackBufferEncoding();
#endif    
    for (auto camera : cameras)
    {
        RenderWithCamera( camera, 0, "Primary Pass" );
    }
    
    GfxDevice::SetRenderTarget( nullptr, 0 );
#if RENDERER_D3D12
    GfxDevice::ClearScreen( GfxDevice::ClearFlags::Depth );
#endif
}

void ae3d::Scene::EndFrame()
{
#if RENDERER_VULKAN
    GfxDevice::EndRenderPassAndCommandBuffer();
#endif
#if RENDERER_METAL
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
#ifndef RENDERER_VULKAN
    GfxDevice::SetViewport( camera->GetViewport() );
#endif
#ifndef RENDERER_METAL
    GfxDevice::SetRenderTarget( camera->GetTargetTexture(), cubeMapFace );
#endif
#ifdef RENDERER_VULKAN
    if (camera->GetTargetTexture())
    {
        BeginOffscreen();
    }

    GfxDevice::SetViewport( camera->GetViewport() );
    GfxDevice::SetScissor( camera->GetViewport() );
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
#if defined( AE3D_OPENVR )
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
#if defined( AE3D_OPENVR )
    view = cameraGo->GetComponent< TransformComponent >()->GetVrView();
    position = Global::vrEyePosition;
    fovDegrees = GetVRFov();
#else
    auto cameraTransform = cameraGo->GetComponent< TransformComponent >();
    position = cameraTransform->GetWorldPosition();
    fovDegrees = camera->GetFovDegrees();
    cameraTransform->GetWorldRotation().GetMatrix( view );
    Matrix44 translation;
    translation.SetTranslation( -cameraTransform->GetWorldPosition() );
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
    int gameObjectIndex = -1;
    
    GfxDeviceGlobal::perObjectUboStruct.lightColor = Vec4( 0, 0, 0, 1 );
    GfxDeviceGlobal::perObjectUboStruct.minAmbient = ambientColor.x;
    GfxDeviceGlobal::perObjectUboStruct.lightType = 0;
    
    for (auto gameObject : gameObjects)
    {
        ++gameObjectIndex;
        
        if (gameObject == nullptr || (gameObject->GetLayer() & camera->GetLayerMask()) == 0 || !gameObject->IsEnabled())
        {
            continue;
        }
        
        auto transform = gameObject->GetComponent< TransformComponent >();
        auto spriteRenderer = gameObject->GetComponent< SpriteRendererComponent >();
        auto dirLight = gameObject->GetComponent< DirectionalLightComponent >();
        
        if (dirLight)
        {
            auto lightTransform = gameObject->GetComponent< TransformComponent >();

            Vec4 lightDirection = Vec4( lightTransform != nullptr ? lightTransform->GetViewDirection() : Vec3( 1, 0, 0 ), 0 );
            Vec3 lightDirectionVS;
            Matrix44::TransformDirection( Vec3( lightDirection.x, lightDirection.y, lightDirection.z ), camera->GetView(), &lightDirectionVS );
            GfxDeviceGlobal::perObjectUboStruct.lightColor = Vec4( dirLight->GetColor() );
            GfxDeviceGlobal::perObjectUboStruct.lightDirection = Vec4( lightDirectionVS.x, lightDirectionVS.y, lightDirectionVS.z, 0 );
            
            // FIXME: This is an ugly hack to get shadow shaders to work with different light types.
            if (dirLight->CastsShadow())
            {
                GfxDeviceGlobal::perObjectUboStruct.lightType = 2;
            }
        }
        else
        {
            GfxDeviceGlobal::perObjectUboStruct.lightType = 1;
        }
        
        if (spriteRenderer)
        {
            Matrix44 localToClip;
            Matrix44::Multiply( transform ? transform->GetLocalToWorldMatrix() : Matrix44::identity, camera->GetProjection(), localToClip );
            spriteRenderer->Render( localToClip.m );
        }
        
        auto textRenderer = gameObject->GetComponent< TextRendererComponent >();
        
        if (textRenderer)
        {
            Matrix44 localToClip;
            Matrix44::Multiply( transform ? transform->GetLocalToWorldMatrix() : Matrix44::identity, camera->GetProjection(), localToClip );
            textRenderer->Render( localToClip.m );
        }
        
        auto meshRenderer = gameObject->GetComponent< MeshRendererComponent >();

        if (meshRenderer)
        {
            gameObjectsWithMeshRenderer.push_back( gameObjectIndex );
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

        Matrix44 localToView;
        Matrix44 localToClip;
        Matrix44::Multiply( meshLocalToWorld, view, localToView );
        Matrix44::Multiply( localToView, camera->GetProjection(), localToClip );

        auto* meshRenderer = gameObjects[ j ]->GetComponent< MeshRendererComponent >();
        meshRenderer->Cull( frustum, meshLocalToWorld );
        meshRenderer->Render( localToView, localToClip, meshLocalToWorld, SceneGlobal::shadowCameraViewMatrix, SceneGlobal::shadowCameraProjectionMatrix, nullptr, nullptr, MeshRendererComponent::RenderType::Opaque );
    }

    for (auto j : gameObjectsWithMeshRenderer)
    {
        auto transform = gameObjects[ j ]->GetComponent< TransformComponent >();
        auto meshLocalToWorld = transform ? transform->GetLocalToWorldMatrix() : Matrix44::identity;
        
        Matrix44 localToView;
        Matrix44 localToClip;
        Matrix44::Multiply( meshLocalToWorld, view, localToView );
        Matrix44::Multiply( localToView, camera->GetProjection(), localToClip );
        
        gameObjects[ j ]->GetComponent< MeshRendererComponent >()->Render( localToView, localToClip, meshLocalToWorld, SceneGlobal::shadowCameraViewMatrix, SceneGlobal::shadowCameraProjectionMatrix, nullptr, nullptr, MeshRendererComponent::RenderType::Transparent );
    }

    GfxDevice::PopGroupMarker();

#if RENDERER_METAL
    GfxDevice::SetRenderTarget( nullptr, 0 );
#endif
#if RENDERER_VULKAN
    GfxDevice::SetRenderTarget( nullptr, 0 );
    
    if (camera->GetTargetTexture())
    {
        EndOffscreen();
    }
#endif
}

void ae3d::Scene::RenderDepthAndNormals( CameraComponent* camera, const Matrix44& worldToView, std::vector< unsigned > gameObjectsWithMeshRenderer,
                                         int cubeMapFace, const Frustum& frustum )
{
#if RENDERER_METAL
    GfxDevice::SetViewport( camera->GetViewport() );
    GfxDevice::ClearScreen( GfxDevice::ClearFlags::Color | GfxDevice::ClearFlags::Depth );
    GfxDevice::SetRenderTarget( &camera->GetDepthNormalsTexture(), cubeMapFace );
#else
    GfxDevice::SetRenderTarget( &camera->GetDepthNormalsTexture(), cubeMapFace );
#if RENDERER_VULKAN
    BeginOffscreen();
    GfxDevice::SetScissor( camera->GetViewport() );
#endif
    GfxDevice::SetViewport( camera->GetViewport() );
    GfxDevice::ClearScreen( GfxDevice::ClearFlags::Color | GfxDevice::ClearFlags::Depth );
#endif
    GfxDevice::PushGroupMarker( "DepthNormal" );

    for (auto j : gameObjectsWithMeshRenderer)
    {
        auto transform = gameObjects[ j ]->GetComponent< TransformComponent >();
        auto meshLocalToWorld = transform ? transform->GetLocalToWorldMatrix() : Matrix44::identity;
        
        Matrix44 localToView;
        Matrix44 localToClip;
        Matrix44::Multiply( meshLocalToWorld, worldToView, localToView );
        Matrix44::Multiply( localToView, camera->GetProjection(), localToClip );
        
        auto meshRenderer = gameObjects[ j ]->GetComponent< MeshRendererComponent >();

        meshRenderer->Cull( frustum, meshLocalToWorld );
        meshRenderer->Render( localToView, localToClip, meshLocalToWorld, SceneGlobal::shadowCameraViewMatrix, SceneGlobal::shadowCameraProjectionMatrix, &renderer.builtinShaders.depthNormalsShader, &renderer.builtinShaders.depthNormalsShader, MeshRendererComponent::RenderType::Opaque );
        meshRenderer->Render( localToView, localToClip, meshLocalToWorld, SceneGlobal::shadowCameraViewMatrix, SceneGlobal::shadowCameraProjectionMatrix, &renderer.builtinShaders.depthNormalsShader,
                             &renderer.builtinShaders.depthNormalsShader, MeshRendererComponent::RenderType::Transparent );
    }

    GfxDevice::PopGroupMarker();
    
    GfxDevice::SetRenderTarget( nullptr, 0 );
#if RENDERER_VULKAN
    EndOffscreen();
#endif
}

void ae3d::Scene::RenderShadowsWithCamera( GameObject* cameraGo, int cubeMapFace )
{
    CameraComponent* camera = cameraGo->GetComponent< CameraComponent >();

    System::Assert( camera->GetTargetTexture() != nullptr, "cannot render shadows if target texture is missing!" );
    int viewport[ 4 ] = { 0, 0, camera->GetTargetTexture()->GetWidth(), camera->GetTargetTexture()->GetHeight() };

#if !RENDERER_METAL
    GfxDevice::SetRenderTarget( camera->GetTargetTexture(), cubeMapFace );
#endif
#ifndef RENDERER_VULKAN
    GfxDevice::SetViewport( viewport );
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
    BeginOffscreen();
    GfxDevice::SetScissor( viewport );
    GfxDevice::SetViewport( viewport );
#endif

    GfxDevice::PushGroupMarker( "Shadow maps" );

    Matrix44 view;
    auto cameraTransform = cameraGo->GetComponent< TransformComponent >();
    cameraTransform->GetWorldRotation().GetMatrix( view );
    Matrix44 translation;
    translation.SetTranslation( -cameraTransform->GetWorldPosition() );
    Matrix44::Multiply( translation, view, view );
    
    SceneGlobal::shadowCameraViewMatrix = view;
    SceneGlobal::shadowCameraProjectionMatrix = camera->GetProjection();
    
    Frustum frustum;
    
    if (camera->GetProjectionType() == CameraComponent::ProjectionType::Perspective)
    {
        frustum.SetProjection( camera->GetFovDegrees(), camera->GetAspect(), camera->GetNear(), camera->GetFar() );
        GfxDeviceGlobal::perObjectUboStruct.lightType = 1;
    }
    else
    {
        frustum.SetProjection( camera->GetLeft(), camera->GetRight(), camera->GetBottom(), camera->GetTop(), camera->GetNear(), camera->GetFar() );
        GfxDeviceGlobal::perObjectUboStruct.lightType = 2;
    }
    
    const Vec3 viewDir = Vec3( view.m[2], view.m[6], view.m[10] ).Normalized();
    frustum.Update( cameraTransform->GetWorldPosition(), viewDir );
    
    std::vector< unsigned > gameObjectsWithMeshRenderer;
    gameObjectsWithMeshRenderer.reserve( gameObjects.size() );
    int gameObjectIndex = -1;
    
    for (auto gameObject : gameObjects)
    {
        ++gameObjectIndex;
        
        if (gameObject == nullptr || !gameObject->IsEnabled())
        {
            continue;
        }
        
        auto meshRenderer = gameObject->GetComponent< MeshRendererComponent >();
        
        if (meshRenderer && meshRenderer->CastsShadow())
        {
            gameObjectsWithMeshRenderer.push_back( gameObjectIndex );
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
        
        Matrix44 localToView;
        Matrix44 localToClip;
        Matrix44::Multiply( meshLocalToWorld, view, localToView );
        Matrix44::Multiply( localToView, camera->GetProjection(), localToClip );

        auto* meshRenderer = gameObjects[ j ]->GetComponent< MeshRendererComponent >();
        
        meshRenderer->Cull( frustum, meshLocalToWorld );
        meshRenderer->Render( localToView, localToClip, meshLocalToWorld, SceneGlobal::shadowCameraViewMatrix, SceneGlobal::shadowCameraProjectionMatrix, &renderer.builtinShaders.momentsShader,
                             &renderer.builtinShaders.momentsSkinShader, MeshRendererComponent::RenderType::Opaque );
    }

    GfxDevice::PopGroupMarker();

#if RENDERER_METAL
    GfxDevice::SetRenderTarget( nullptr, 0 );
#endif
#if RENDERER_VULKAN
    EndOffscreen();
#endif
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
        
        if (gameObject->GetComponent<MeshRendererComponent>())
        {
            outSerialized += ::GetSerialized( gameObject->GetComponent<MeshRendererComponent>() );
        }
        if (gameObject->GetComponent<TransformComponent>())
        {
            outSerialized += ::GetSerialized( gameObject->GetComponent<TransformComponent>() );
        }
        if (gameObject->GetComponent<CameraComponent>())
        {
            outSerialized += ::GetSerialized( gameObject->GetComponent<CameraComponent>() );
        }
        if (gameObject->GetComponent<SpriteRendererComponent>())
        {
            outSerialized += gameObject->GetComponent<SpriteRendererComponent>()->GetSerialized();
        }
        if (gameObject->GetComponent<TextRendererComponent>())
        {
            outSerialized += ::GetSerialized( gameObject->GetComponent<TextRendererComponent>() );
        }
        if (gameObject->GetComponent<AudioSourceComponent>())
        {
            outSerialized += ::GetSerialized( gameObject->GetComponent<AudioSourceComponent>() );
        }
        if (gameObject->GetComponent<DirectionalLightComponent>())
        {
            outSerialized += ::GetSerialized( gameObject->GetComponent<DirectionalLightComponent>() );
        }
        if (gameObject->GetComponent<SpotLightComponent>())
        {
            outSerialized += ::GetSerialized( gameObject->GetComponent<SpotLightComponent>() );
        }
        if (gameObject->GetComponent<PointLightComponent>())
        {
            outSerialized += ::GetSerialized( gameObject->GetComponent<PointLightComponent>() );
        }
    }
    return outSerialized;
}

ae3d::Scene::DeserializeResult ae3d::Scene::Deserialize( const FileSystem::FileContentsData& serialized, std::vector< GameObject >& outGameObjects,
                                                        std::map< std::string, Texture2D* >& outTexture2Ds,
                                                        std::map< std::string, Material* >& outMaterials,
                                                        Array< Mesh* >& outMeshes ) const
{
    // TODO: It would be better to store the token strings into somewhere accessible to GetSerialized() to prevent typos etc.

    outGameObjects.clear();

    std::stringstream stream( std::string( std::begin( serialized.data ), std::end( serialized.data ) ) );
    std::string line;
    
    std::string currentMaterialName;
    
    enum class CurrentLightType { Directional, Spot, Point, None };
    CurrentLightType currentLightType = CurrentLightType::None;
    
    // FIXME: These ensure that the mesh is rendered. A proper fix would be to serialize materials.
    static Shader tempShader;
    tempShader.Load( "unlit_vertex", "unlit_fragment",
        FileSystem::FileContents( "unlit_vert.obj" ), FileSystem::FileContents( "unlit_frag.obj" ),
        FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );

    Material* tempMaterial = new Material();
    tempMaterial->SetShader( &tempShader );
    tempMaterial->SetTexture( Texture2D::GetDefaultTexture(), 0 );
    tempMaterial->SetVector( "tint", { 1, 1, 1, 1 } );
    tempMaterial->SetBackFaceCulling( true );
    outMaterials[ "temp material" ] = tempMaterial;

    int lineNo = 0;
    while (!stream.eof())
    {
        std::getline( stream, line );
        std::stringstream lineStream( line );
        std::locale c_locale( "C" );
        lineStream.imbue( c_locale );
        std::string token;
        lineStream >> token;
        
        ++lineNo;
        
        if (token == "gameobject")
        {
            outGameObjects.push_back( GameObject() );
            std::string name;
            lineStream >> name;
            outGameObjects.back().SetName( name.c_str() );
        }
        else if (token.empty())
        {
        }
        else if (token == "name")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found name but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
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
                System::Print( "Failed to parse %s at line %d: found layer but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
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
                System::Print( "Failed to parse %s at line %d: found layer but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            int enabled;
            lineStream >> enabled;
            outGameObjects.back().SetEnabled( enabled != 0 );
        }
        else if (token == "meshrenderer_enabled")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found meshrenderer_enabled but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            int enabled;
            lineStream >> enabled;

            auto meshRenderer = outGameObjects.back().GetComponent< MeshRendererComponent >();

            if (meshRenderer == nullptr)
            {
                System::Print( "Failed to parse %s at line %d: found meshrenderer_enabled but the game object doesn't have a mesh renderer component.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            meshRenderer->SetEnabled( enabled != 0 );
        }
        else if (token == "transform_enabled")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found transform_enabled but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            int enabled;
            lineStream >> enabled;

            auto transform = outGameObjects.back().GetComponent< TransformComponent >();

            if (transform == nullptr)
            {
                System::Print( "Failed to parse %s at line %d: found transform_enabled but the game object doesn't have a transform component.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            transform->SetEnabled( enabled != 0 );
        }
        else if (token == "camera_enabled")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found camera_enabled but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            int enabled;
            lineStream >> enabled;

            auto camera = outGameObjects.back().GetComponent< CameraComponent >();

            if (camera == nullptr)
            {
                System::Print( "Failed to parse %s at line %d: found camera_enabled but the game object doesn't have a camera component.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            camera->SetEnabled( enabled != 0 );
        }
        else if (token == "dirlight")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found dirlight but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
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
                System::Print( "Failed to parse %s at line %d: found spotlight but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
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
                System::Print( "Failed to parse %s at line %d: found pointlight but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
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
                System::Print( "Failed to parse %s at line %d: found shadow but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
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
                System::Print( "Failed to parse %s at line %d: found camera but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            outGameObjects.back().AddComponent< CameraComponent >();
        }
        else if (token == "ortho")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found ortho but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
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
                System::Print( "Failed to parse %s at line %d: found persp but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
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
                System::Print( "Failed to parse %s at line %d: found projection but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
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
                System::Print( "Failed to parse %s at line %d: found clearcolor but there are no game objects defined before this line.\n", serialized.path.c_str() , lineNo );
                return DeserializeResult::ParseError;
            }

            float red, green, blue;
            lineStream >> red >> green >> blue;
            outGameObjects.back().GetComponent< CameraComponent >()->SetClearColor( { red, green, blue } );
        }
        else if (token == "layermask")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found layermask but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            unsigned layerMask;
            lineStream >> layerMask;
            outGameObjects.back().GetComponent< CameraComponent >()->SetLayerMask( layerMask );
        }
        else if (token == "viewport")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found viewport but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            unsigned x, y, w, h;
            lineStream >> x >> y >> w >> h;
            outGameObjects.back().GetComponent< CameraComponent >()->SetViewport( x, y, w, h );
        }
        else if (token == "order")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found order but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            unsigned order = 0;
            lineStream >> order;
            outGameObjects.back().GetComponent< CameraComponent >()->SetRenderOrder( order );
        }
        else if (token == "transform")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found transform but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            outGameObjects.back().AddComponent< TransformComponent >();
        }
        else if (token == "meshrenderer_cast_shadow")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found meshrenderer_cast_shadow but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            auto meshRenderer = outGameObjects.back().GetComponent< MeshRendererComponent >();

            if (meshRenderer == nullptr)
            {
                System::Print( "Failed to parse %s at line %d: found mesh_cast_shadow but the game object doesn't have a mesh renderer component.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            std::string str;
            lineStream >> str;
            meshRenderer->SetCastShadow( str == std::string( "1" ) );
        }
        else if (token == "meshrenderer")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found meshrenderer but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            outGameObjects.back().AddComponent< MeshRendererComponent >();
        }
        else if (token == "meshpath")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found meshFile but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            auto meshRenderer = outGameObjects.back().GetComponent< MeshRendererComponent >();

            if (!meshRenderer)
            {
                System::Print( "Failed to parse %s at line %d: found meshpath but the game object doesn't have a mesh renderer component.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            std::string meshFile;
            lineStream >> meshFile;

            Mesh* mesh = new Mesh();
            outMeshes.Add( mesh );

            outMeshes[ outMeshes.count - 1 ]->Load( FileSystem::FileContents( meshFile.c_str() ) );
			meshRenderer->SetMesh( outMeshes[ outMeshes.count - 1 ] );

            meshRenderer->SetMaterial( tempMaterial, 0 );
        }
        else if (token == "spriterenderer")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found spriterenderer but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            outGameObjects.back().AddComponent< SpriteRendererComponent >();
        }
        else if (token == "sprite")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found sprite but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            if (!outGameObjects.back().GetComponent< SpriteRendererComponent >())
            {
                System::Print( "Failed to parse %s at line %d: found sprite but the game object doesn't have a sprite renderer component.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            std::string spritePath;
            float x, y, width, height;
            lineStream >> spritePath >> x >> y >> width >> height;

            outTexture2Ds[ spritePath ] = new Texture2D();
            outTexture2Ds[ spritePath ]->Load( FileSystem::FileContents( spritePath.c_str() ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );

            outGameObjects.back().GetComponent< SpriteRendererComponent >()->SetTexture( outTexture2Ds[ spritePath ], Vec3( x, y, 0 ), Vec3( x, y, 1 ), Vec4( 1, 1, 1, 1 ) );
        }
        else if (token == "position")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found position but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            if (!outGameObjects.back().GetComponent< TransformComponent >())
            {
                System::Print( "Failed to parse %s at line %d: found position but the game object doesn't have a transform component.\n", serialized.path.c_str(), lineNo );
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
                System::Print( "Failed to parse %s at line %d: found rotation but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            if (!outGameObjects.back().GetComponent< TransformComponent >())
            {
                System::Print( "Failed to parse %s at line %d: found rotation but the game object doesn't have a transform component.\n", serialized.path.c_str(), lineNo );
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
                System::Print( "Failed to parse %s at line %d: found scale but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }
            
            if (!outGameObjects.back().GetComponent< TransformComponent >())
            {
                System::Print( "Failed to parse %s at line %d: found scale but the game object doesn't have a transform component.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }

            float objScale;
            lineStream >> objScale;
            outGameObjects.back().GetComponent< TransformComponent >()->SetLocalScale( objScale );
        }
        else if (token == "texture2d")
        {
            std::string name;
            std::string path;
            
            lineStream >> name >> path;
            
            if (!lineStream.eof())
            {
                std::string compressedTexturePath;
                lineStream >> compressedTexturePath;
                
#if !TARGET_OS_IPHONE
                if (compressedTexturePath.find( ".dds" ) != std::string::npos)
                {
                    path = compressedTexturePath;
                }
                else if (!lineStream.eof())
                {
                    lineStream >> compressedTexturePath;
                    
                    if (compressedTexturePath.find( ".dds" ) != std::string::npos)
                    {
                        path = compressedTexturePath;
                    }
                }
#endif
#if TARGET_OS_IPHONE
                if (compressedTexturePath.find( ".astc" ) != std::string::npos)
                {
                    path = compressedTexturePath;
                }
                else if (!lineStream.eof())
                {
                    lineStream >> compressedTexturePath;
                    
                    if (compressedTexturePath.find( ".astc" ) != std::string::npos)
                    {
                        path = compressedTexturePath;
                    }
                }
#endif
            }

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
                System::Print( "Failed to parse %s at line %d: found mesh_material but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }
            
            auto mr = outGameObjects.back().GetComponent< MeshRendererComponent >();

            if (!mr)
            {
                System::Print( "Failed to parse %s at line %d: found mesh_material but the last defined game object doesn't have a mesh renderer component.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }
            
            if (!mr->GetMesh())
            {
                System::Print( "Failed to parse %s at line %d: found mesh_material but the last defined game object's mesh renderer doesn't have a mesh.\n", serialized.path.c_str(), lineNo );
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
            
            outMaterials[ currentMaterialName ]->SetTexture( outTexture2Ds[ textureName ], 0 );
        }
        else if (token == "audiosource")
        {
            if (outGameObjects.empty())
            {
                System::Print( "Failed to parse %s at line %d: found audiosource but there are no game objects defined before this line.\n", serialized.path.c_str(), lineNo );
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
            
            if (currentLightType == CurrentLightType::Point)
            {
                outGameObjects.back().GetComponent< PointLightComponent >()->SetRadius( radius );
            }
            else if (currentLightType == CurrentLightType::Spot)
            {
                outGameObjects.back().GetComponent< SpotLightComponent >()->SetRadius( radius );
            }
            else
            {
                System::Print( "Found 'radius' but no spotlight or point light is defined before this line.\n" );
            }
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
        else if (token == "shaders")
        {
            if (currentMaterialName.empty())
            {
                System::Print( "Failed to parse %s at line %d: found 'metal_shaders' but there are no materials defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }
            
            std::string vertexShaderName, fragmentShaderName;
            lineStream >> vertexShaderName >> fragmentShaderName;

            std::string hlslVert = vertexShaderName + std::string( ".obj" );
            std::string hlslFrag = fragmentShaderName + std::string( ".obj" );
            std::string spvVert = vertexShaderName + std::string( "_vert.spv" );
            std::string spvFrag = fragmentShaderName + std::string( "_frag.fsh" );
            
            Shader* shader = new Shader();
            shader->Load( vertexShaderName.c_str(), fragmentShaderName.c_str(),
                          FileSystem::FileContents( hlslVert.c_str() ), FileSystem::FileContents( hlslFrag.c_str() ),
                          FileSystem::FileContents( spvVert.c_str() ), FileSystem::FileContents( spvFrag.c_str() ) );

            outMaterials[ currentMaterialName ]->SetShader( shader );
        }
        else if (token == "metal_shaders")
        {
#if RENDERER_METAL
            if (currentMaterialName == "")
            {
                System::Print( "Failed to parse %s at line %d: found 'metal_shaders' but there are no materials defined before this line.\n", serialized.path.c_str(), lineNo );
                return DeserializeResult::ParseError;
            }
            
            std::string vertexShaderName, fragmentShaderName;
            lineStream >> vertexShaderName >> fragmentShaderName;

            Shader* shader = new Shader();
            shader->Load(   vertexShaderName.c_str(), fragmentShaderName.c_str(),
                            FileSystem::FileContents( "unlit.hlsl" ), FileSystem::FileContents( "unlit.hlsl" ),
                            FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );

            outMaterials[ currentMaterialName ]->SetShader( shader );
#endif
        }
        else
        {
            System::Print( "Scene parser: Unhandled token '%s' at line %d\n", token.c_str(), lineNo );
        }
    }

    for (const auto& go : outGameObjects)
    {
        const auto mr = go.GetComponent< MeshRendererComponent >();

        if (mr)
        {
            const unsigned subMeshCount = mr->GetMesh()->GetSubMeshCount();
            
            for (unsigned i = 0; i < subMeshCount; ++i)
            {
                if (!mr->GetMaterial( i ))
                {
                    System::Print( "Scene parser: missing material for mesh renderer in game object %s in subMesh %s\n", go.GetName(), mr->GetMesh()->GetSubMeshName( i ) );
                }
            }
        }
    }
    
    return DeserializeResult::Success;
}

void ae3d::Scene::GenerateAABB()
{
    Statistics::BeginSceneAABB();
    
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
    
    Statistics::EndSceneAABB();
}
