#if AE3D_OPENVR
#include "VR.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <cstring>
#include <openvr.h>
#include <vulkan/vulkan.h>
#include "CameraComponent.hpp"
#include "GameObject.hpp"
#include "FileSystem.hpp"
#include "Matrix.hpp"
#include "Macros.hpp"
#include "System.hpp"
#include "Shader.hpp"
#include "TransformComponent.hpp"
#include "Vec3.hpp"

using namespace ae3d;

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkPhysicalDevice physicalDevice;
    extern VkInstance instance;
    extern VkQueue graphicsQueue;
    extern std::uint32_t graphicsQueueIndex;
}

struct FramebufferDesc
{
    int width;
    int height;
};

struct Vector2
{
    Vector2() : x( 0 ), y( 0 ) {}
    Vector2( float aX, float aY ) : x( aX ), y( aY ) {}

    float x, y;
};

struct VertexDataLens
{
    Vector2 position;
    Vector2 texCoordRed;
    Vector2 texCoordGreen;
    Vector2 texCoordBlue;
};

namespace WindowGlobal
{
    extern int windowWidth;
    extern int windowHeight;
}

namespace Global
{
    vr::IVRSystem* hmd = nullptr;
    vr::IVRRenderModels* renderModels = nullptr;
    std::uint32_t width, height;
    FramebufferDesc leftEyeDesc;
    FramebufferDesc rightEyeDesc;
    unsigned int indexSize;
    float vrFov = 45;
    vr::TrackedDevicePose_t trackedDevicePose[ vr::k_unMaxTrackedDeviceCount ];
    int validPoseCount;
    Matrix44 mat4HMDPose;
    Matrix44 eyePosLeft;
    Matrix44 eyePosRight;
    std::string poseClasses;
    char devClassChar[ vr::k_unMaxTrackedDeviceCount ];   // for each device, a character representing its class
    Matrix44 devicePose[ vr::k_unMaxTrackedDeviceCount ];
    Shader lensDistort;
    Vec3 vrEyePosition; // Used in Scene.cpp for frustum culling
    Vec3 leftControllerPosition;
    Vec3 rightControllerPosition;
    int leftHandIndex = -1;
    int rightHandIndex = -1;
    VkPipeline companionPSO;
    VkVertexInputAttributeDescription inputCompanion;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkVertexInputAttributeDescription attributeDescriptions[ 3 ];
}

float GetVRFov()
{
    return Global::vrFov;
}

bool CreateFrameBuffer( int width, int height, FramebufferDesc& outFramebufferDesc )
{
    outFramebufferDesc.width = width;
    outFramebufferDesc.height = height;
    return true;
}

void SetupDistortion()
{
    const short lensGridSegmentCountH = 43;
    const short lensGridSegmentCountV = 43;

    const float w = (float)(1.0f / float( lensGridSegmentCountH - 1 ));
    const float h = (float)(1.0f / float( lensGridSegmentCountV - 1 ));

    float u, v = 0;

    std::vector< VertexDataLens > vVerts( 0 );
    VertexDataLens vert;

    // Left eye distortion verts
    float Xoffset = -1;
    for (int y = 0; y < lensGridSegmentCountV; ++y)
    {
        for (int x = 0; x < lensGridSegmentCountH; ++x)
        {
            u = x * w; v = 1 - y * h;
            vert.position = Vector2( Xoffset + u, -1 + 2 * y*h );

            vr::DistortionCoordinates_t dc0;
            const bool success = Global::hmd->ComputeDistortion( vr::Eye_Left, u, v, &dc0 );

            vert.texCoordRed = Vector2( dc0.rfRed[ 0 ], 1 - dc0.rfRed[ 1 ] );
            vert.texCoordGreen = Vector2( dc0.rfGreen[ 0 ], 1 - dc0.rfGreen[ 1 ] );
            vert.texCoordBlue = Vector2( dc0.rfBlue[ 0 ], 1 - dc0.rfBlue[ 1 ] );

            vVerts.push_back( vert );
        }
    }

    // Right eye distortion verts
    Xoffset = 0;
    for (int y = 0; y < lensGridSegmentCountV; ++y)
    {
        for (int x = 0; x < lensGridSegmentCountH; ++x)
        {
            u = x * w; v = 1 - y * h;
            vert.position = Vector2( Xoffset + u, -1 + 2 * y * h );

            vr::DistortionCoordinates_t dc0;
            const bool success = Global::hmd->ComputeDistortion( vr::Eye_Right, u, v, &dc0 );

            vert.texCoordRed = Vector2( dc0.rfRed[ 0 ], 1 - dc0.rfRed[ 1 ] );
            vert.texCoordGreen = Vector2( dc0.rfGreen[ 0 ], 1 - dc0.rfGreen[ 1 ] );
            vert.texCoordBlue = Vector2( dc0.rfBlue[ 0 ], 1 - dc0.rfBlue[ 1 ] );

            vVerts.push_back( vert );
        }
    }

    std::vector<short> vIndices;
    short a, b, c, d;

    short offset = 0;
    for (short y = 0; y < lensGridSegmentCountV - 1; ++y)
    {
        for (short x = 0; x < lensGridSegmentCountH - 1; ++x)
        {
            a = lensGridSegmentCountH * y + x + offset;
            b = lensGridSegmentCountH * y + x + 1 + offset;
            c = (y + 1) * lensGridSegmentCountH + x + 1 + offset;
            d = (y + 1) * lensGridSegmentCountH + x + offset;
            vIndices.push_back( a );
            vIndices.push_back( b );
            vIndices.push_back( c );

            vIndices.push_back( a );
            vIndices.push_back( c );
            vIndices.push_back( d );
        }
    }

    offset = lensGridSegmentCountH * lensGridSegmentCountV;
    for (short y = 0; y < lensGridSegmentCountV - 1; ++y)
    {
        for (short x = 0; x < lensGridSegmentCountH - 1; ++x)
        {
            a = lensGridSegmentCountH * y + x + offset;
            b = lensGridSegmentCountH * y + x + 1 + offset;
            c = (y + 1) * lensGridSegmentCountH + x + 1 + offset;
            d = (y + 1) * lensGridSegmentCountH + x + offset;
            vIndices.push_back( a );
            vIndices.push_back( b );
            vIndices.push_back( c );

            vIndices.push_back( a );
            vIndices.push_back( c );
            vIndices.push_back( d );
        }
    }
    
    Global::indexSize = static_cast< unsigned int >( vIndices.size() );
}

std::string GetTrackedDeviceString( vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL )
{
    std::uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty( unDevice, prop, NULL, 0, peError );
    if (unRequiredBufferLen == 0)
    {
        return "";
    }

    char* pchBuffer = new char[ unRequiredBufferLen ];
    unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty( unDevice, prop, pchBuffer, unRequiredBufferLen, peError );
    std::string sResult = pchBuffer;
    delete[] pchBuffer;
    return sResult;
}

void ConvertSteamVRMatrixToMatrix4( const vr::HmdMatrix34_t &steamMatrix, Matrix44& outMatrix )
{
    outMatrix.m[ 0 ] = steamMatrix.m[ 0 ][ 0 ];
    outMatrix.m[ 1 ] = steamMatrix.m[ 0 ][ 1 ];
    outMatrix.m[ 2 ] = steamMatrix.m[ 0 ][ 2 ];
    outMatrix.m[ 3 ] = steamMatrix.m[ 0 ][ 3 ];
    outMatrix.m[ 4 ] = steamMatrix.m[ 1 ][ 0 ];
    outMatrix.m[ 5 ] = steamMatrix.m[ 1 ][ 1 ];
    outMatrix.m[ 6 ] = steamMatrix.m[ 1 ][ 2 ];
    outMatrix.m[ 7 ] = steamMatrix.m[ 1 ][ 3 ];
    outMatrix.m[ 8 ] = steamMatrix.m[ 2 ][ 0 ];
    outMatrix.m[ 9 ] = steamMatrix.m[ 2 ][ 1 ];
    outMatrix.m[10 ] = steamMatrix.m[ 2 ][ 2 ];
    outMatrix.m[11 ] = steamMatrix.m[ 2 ][ 3 ];
    outMatrix.m[12 ] = 0;
    outMatrix.m[13 ] = 0;
    outMatrix.m[14 ] = 0;
    outMatrix.m[15 ] = 1;

    outMatrix.Transpose( outMatrix );
}

void RenderDistortion()
{
    System::Assert( Global::lensDistort.IsValid(), "lens distortion shader is not valid" );
}

void SetupDescriptors()
{
    VkDescriptorSetLayoutBinding layoutBindings[ 3 ] = {};
    layoutBindings[ 0 ].binding = 0;
    layoutBindings[ 0 ].descriptorCount = 1;
    layoutBindings[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[ 0 ].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    layoutBindings[ 1 ].binding = 1;
    layoutBindings[ 1 ].descriptorCount = 1;
    layoutBindings[ 1 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    layoutBindings[ 1 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    layoutBindings[ 2 ].binding = 2;
    layoutBindings[ 2 ].descriptorCount = 1;
    layoutBindings[ 2 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    layoutBindings[ 2 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptorSetLayoutCreateInfo.bindingCount = 3;
    descriptorSetLayoutCreateInfo.pBindings = &layoutBindings[ 0 ];
    VkResult err = vkCreateDescriptorSetLayout( GfxDeviceGlobal::device, &descriptorSetLayoutCreateInfo, nullptr, &Global::descriptorSetLayout );
    AE3D_CHECK_VULKAN( err, "Unable to create descriptor set layout" );

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &Global::descriptorSetLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    err = vkCreatePipelineLayout( GfxDeviceGlobal::device, &pipelineLayoutCreateInfo, nullptr, &Global::pipelineLayout );
    AE3D_CHECK_VULKAN( err, "Unable to create pipeline layout" );

    Global::attributeDescriptions[ 0 ].binding = 0;
    Global::attributeDescriptions[ 0 ].location = 0;
    Global::attributeDescriptions[ 0 ].format = VK_FORMAT_R32G32_SFLOAT;
    Global::attributeDescriptions[ 0 ].offset = 0;

    Global::attributeDescriptions[ 1 ].binding = 1;
    Global::attributeDescriptions[ 1 ].location = 0;
    Global::attributeDescriptions[ 1 ].format = VK_FORMAT_R32G32_SFLOAT;
    Global::attributeDescriptions[ 1 ].offset = sizeof( float ) * 2;

    Global::attributeDescriptions[ 2 ].binding = 0;
    Global::attributeDescriptions[ 2 ].location = 0;
    Global::attributeDescriptions[ 2 ].format = VK_FORMAT_UNDEFINED;
    Global::attributeDescriptions[ 2 ].offset = 0;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
}

Vec3 ae3d::VR::GetRightHandPosition()
{
    return Global::rightControllerPosition;
}

Vec3 ae3d::VR::GetLeftHandPosition()
{
    return Global::leftControllerPosition;
}

void ae3d::VR::Init()
{
    vr::EVRInitError eError = vr::VRInitError_None;
    Global::hmd = vr::VR_Init( &eError, vr::VRApplication_Scene );

    if (eError != vr::VRInitError_None)
    {
        System::Print( "Unable to init OpenVR: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription( eError ) );
        return;
    }

    Global::renderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, &eError );

    if (eError != vr::VRInitError_None)
    {
        System::Print( "Unable to get OpenVR render models: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription( eError ) );
    }

    std::string driver = GetTrackedDeviceString( Global::hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String );
    std::string display = GetTrackedDeviceString( Global::hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String );
    System::Print( "OpenVR driver: %s, display: %s\n", driver.c_str(), display.c_str() );

    Global::hmd->GetRecommendedRenderTargetSize( &Global::width, &Global::height );
    System::Print( "OpenVR width: %d, height: %d\n", Global::width, Global::height );

    CreateFrameBuffer( Global::width, Global::height, Global::leftEyeDesc );
    CreateFrameBuffer( Global::width, Global::height, Global::rightEyeDesc );

    Global::lensDistort.LoadSPIRV( ae3d::FileSystem::FileContents( "vr_companion_vert.spv" ), ae3d::FileSystem::FileContents( "vr_companion_vert.spv" ) );
    SetupDescriptors();
    SetupDistortion();

    if (Global::hmd)
    {
        vr::HmdMatrix34_t matEye = Global::hmd->GetEyeToHeadTransform( vr::Eye_Left );
        ConvertSteamVRMatrixToMatrix4( matEye, Global::eyePosLeft );
        
        matEye = Global::hmd->GetEyeToHeadTransform( vr::Eye_Right );
        ConvertSteamVRMatrixToMatrix4( matEye, Global::eyePosRight );
    }
}

void ae3d::VR::Deinit()
{
    System::Print("window size: %d x %d\n", WindowGlobal::windowWidth, WindowGlobal::windowHeight );

    if (Global::hmd)
    {
        vr::VR_Shutdown();
        Global::hmd = nullptr;
    }
}

void ae3d::VR::GetIdealWindowSize( int& outWidth, int& outHeight )
{
    if (!Global::hmd)
    {
        return;
    }

    std::uint32_t width, height;
    Global::hmd->GetRecommendedRenderTargetSize( &width, &height );

    outWidth = static_cast< int >( width );
    outHeight = static_cast< int >( height );
}

void ae3d::VR::StartTracking( int /*windowWidth*/, int /*windowHeight*/ )
{
}

void ae3d::VR::RecenterTracking()
{
}

void ae3d::VR::SubmitFrame()
{
    if (!Global::hmd)
    {
        return;
    }

    for (vr::TrackedDeviceIndex_t controllerIndex = 0; controllerIndex < vr::k_unMaxTrackedDeviceCount; ++controllerIndex)
    {
        vr::VRControllerState_t state;

        // button 2 is the one above the "trackpad"
        if (Global::hmd->GetControllerState( controllerIndex, &state, sizeof( state ) ))
        {
            //System::Print( "button state for controller %d: %ld\n", controllerIndex, state.ulButtonPressed );
            //m_rbShowTrackedDevice[ unDevice ] = state.ulButtonPressed == 0;
        }
    }

    RenderDistortion();

    vr::VRVulkanTextureData_t vulkanData;
    //vulkanData.m_nImage = (uint64_t)Global::leftEyeDesc.m_pImage;
    vulkanData.m_pDevice = (VkDevice_T *)GfxDeviceGlobal::device;
    vulkanData.m_pPhysicalDevice = (VkPhysicalDevice_T *)GfxDeviceGlobal::physicalDevice;
    vulkanData.m_pInstance = (VkInstance_T *)GfxDeviceGlobal::instance;
    vulkanData.m_pQueue = (VkQueue_T *)GfxDeviceGlobal::graphicsQueue;
    vulkanData.m_nQueueFamilyIndex = GfxDeviceGlobal::graphicsQueueIndex;

    vulkanData.m_nWidth = Global::leftEyeDesc.width;
    vulkanData.m_nHeight = Global::leftEyeDesc.height;
    vulkanData.m_nFormat = VK_FORMAT_R8G8B8A8_SRGB;
    //vulkanData.m_nSampleCount = m_nMSAASampleCount;

    vr::Texture_t texture = { &vulkanData, vr::TextureType_Vulkan, vr::ColorSpace_Auto };
    vr::EVRCompositorError submitResult = vr::VRCompositor()->Submit( vr::Eye_Left, &texture, nullptr );

    if (submitResult != vr::VRCompositorError_None)
    {
        System::Print( "VR submit for left eye returned error %d\n", submitResult );
    }

    //vulkanData.m_nImage = (uint64_t)Global::rightEyeDesc.m_pImage;
    vulkanData.m_nWidth = Global::rightEyeDesc.width;
    vulkanData.m_nHeight = Global::rightEyeDesc.height;

    submitResult = vr::VRCompositor()->Submit( vr::Eye_Right, &texture, nullptr );
    
    if (submitResult != vr::VRCompositorError_None)
    {
        System::Print( "VR submit for right eye returned error %d\n", submitResult );
    }

    vr::VRCompositor()->PostPresentHandoff();
}

void ae3d::VR::SetEye( int eye )
{
    if (!Global::hmd)
    {
        return;
    }

    if (eye == 0)
    {
        //glClearColor( 0.15f, 0.15f, 0.58f, 1.0f );
    }
    if (eye == 1)
    {
        //glClearColor( 0.15f, 0.55f, 0.18f, 1.0f );
    }
}

void ae3d::VR::UnsetEye( int eye )
{
    if (!Global::hmd)
    {
        return;
    }    
}

void ae3d::VR::CalcEyePose()
{
    if (!Global::hmd)
    {
        return;
    }

    vr::VRCompositor()->WaitGetPoses( Global::trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0 );

    Global::validPoseCount = 0;
    Global::poseClasses = "";

    for (unsigned deviceIndex = 0; deviceIndex < vr::k_unMaxTrackedDeviceCount; ++deviceIndex)
    {
        if (Global::trackedDevicePose[ deviceIndex ].bPoseIsValid)
        {
            ++Global::validPoseCount;
            ConvertSteamVRMatrixToMatrix4( Global::trackedDevicePose[ deviceIndex ].mDeviceToAbsoluteTracking, Global::devicePose[ deviceIndex ] );
            
            if (Global::devClassChar[ deviceIndex ] == 0)
            {
                switch (Global::hmd->GetTrackedDeviceClass( deviceIndex ))
                {
                case vr::TrackedDeviceClass_Controller:        Global::devClassChar[ deviceIndex ] = 'C'; break;
                case vr::TrackedDeviceClass_HMD:               Global::devClassChar[ deviceIndex ] = 'H'; break;
                case vr::TrackedDeviceClass_Invalid:           Global::devClassChar[ deviceIndex ] = 'I'; break;
                case vr::TrackedDeviceClass_GenericTracker:    Global::devClassChar[ deviceIndex ] = 'O'; break;
                case vr::TrackedDeviceClass_TrackingReference: Global::devClassChar[ deviceIndex ] = 'T'; break;
                default:                                       Global::devClassChar[ deviceIndex ] = '?'; break;
                }
            }

            Global::poseClasses += Global::devClassChar[ deviceIndex ];
        }
    }

    for (unsigned deviceIndex = 0; deviceIndex < vr::k_unMaxTrackedDeviceCount; ++deviceIndex)
    {
        if (Global::devClassChar[ deviceIndex ] == 'C' && Global::leftHandIndex == -1)
        {
            Global::leftHandIndex = deviceIndex;
        }
        else if (Global::devClassChar[ deviceIndex ] == 'C' && Global::rightHandIndex == -1)
        {
            Global::rightHandIndex = deviceIndex;
        }
    }

    if (Global::rightHandIndex != -1 && Global::trackedDevicePose[ Global::rightHandIndex ].bPoseIsValid)
    {
        Global::rightControllerPosition = Vec3( Global::devicePose[ Global::rightHandIndex ].m[ 12 ],
                                                Global::devicePose[ Global::rightHandIndex ].m[ 13 ],
                                                Global::devicePose[ Global::rightHandIndex ].m[ 14 ] );
        //System::Print( "right hand position: %f %f %f\n", Global::rightControllerPosition.x, Global::rightControllerPosition.y, Global::rightControllerPosition.z );
    }

    if (Global::leftHandIndex != -1 && Global::trackedDevicePose[ Global::leftHandIndex ].bPoseIsValid)
    {
        Global::leftControllerPosition = Vec3( Global::devicePose[ Global::leftHandIndex ].m[ 12 ],
                                               Global::devicePose[ Global::leftHandIndex ].m[ 13 ],
                                               Global::devicePose[ Global::leftHandIndex ].m[ 14 ] );
    }

    if (Global::trackedDevicePose[ vr::k_unTrackedDeviceIndex_Hmd ].bPoseIsValid)
    {
        Global::vrEyePosition = Vec3( Global::devicePose[ vr::k_unTrackedDeviceIndex_Hmd ].m[ 12 ],
                                      Global::devicePose[ vr::k_unTrackedDeviceIndex_Hmd ].m[ 13 ],
                                      Global::devicePose[ vr::k_unTrackedDeviceIndex_Hmd ].m[ 14 ] );
        Matrix44::Invert( Global::devicePose[ vr::k_unTrackedDeviceIndex_Hmd ], Global::mat4HMDPose );
    }
}

void ae3d::VR::CalcCameraForEye( GameObject& camera, float /*yawDegrees*/, int eye )
{
    if (!Global::hmd || !camera.GetComponent< TransformComponent >() || !camera.GetComponent< CameraComponent >())
    {
        return;
    }

    const float nearPlane = camera.GetComponent< CameraComponent >()->GetNear();
    const float farPlane = camera.GetComponent< CameraComponent >()->GetFar();

    vr::HmdMatrix44_t mat = Global::hmd->GetProjectionMatrix( eye == 0 ? vr::Hmd_Eye::Eye_Left : vr::Hmd_Eye::Eye_Right, nearPlane, farPlane );
    Matrix44 projMat;
    std::memcpy( &projMat.m[ 0 ], &mat.m[ 0 ][ 0 ], sizeof( Matrix44 ) );
    projMat.Transpose( projMat );

    Matrix44 view;
    Matrix44::Multiply( Global::mat4HMDPose, eye == 0 ? Global::eyePosLeft : Global::eyePosRight, view );
    camera.GetComponent< TransformComponent >()->SetVrView( view );
    camera.GetComponent< CameraComponent >()->SetProjection( projMat );

    Global::vrFov = std::atan( Global::height / nearPlane ) * (180.0f / 3.14159265f);
    //System::Print("vrFor: %f\n", Global::vrFov );
}
#endif
