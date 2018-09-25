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
#include "VulkanUtils.hpp"

using namespace ae3d;

namespace VRGlobal
{
    int eye = 0;
}

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkPhysicalDevice physicalDevice;
    extern VkInstance instance;
    extern VkQueue graphicsQueue;
    extern VkCommandBuffer offscreenCmdBuffer;
    extern std::uint32_t graphicsQueueIndex;
}

struct FramebufferDesc
{
    VkImage image;
    VkImageView imageView;
    VkDeviceMemory deviceMemory;
    VkImage depthStencilImage;
    VkImageView depthStencilImageView;
    VkDeviceMemory depthStencilDeviceMemory;
    VkFramebuffer framebuffer;
    VkRenderPass renderPass;
    VkImageLayout imageLayout;
    VkImageLayout depthStencilImageLayout;
    int width;
    int height;
};

struct Vector2
{
    Vector2() noexcept : x( 0 ), y( 0 ) {}
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

bool CreateFrameBuffer( int width, int height, FramebufferDesc& outFramebufferDesc, const char* debugName )
{
    outFramebufferDesc.width = width;
    outFramebufferDesc.height = height;

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.usage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    imageCreateInfo.flags = 0;

    VkResult result = vkCreateImage( GfxDeviceGlobal::device, &imageCreateInfo, nullptr, &outFramebufferDesc.image );
    if (result != VK_SUCCESS)
    {
        System::Print( "Failed to create a framebuffer!\n" );
        return false;
    }

    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)outFramebufferDesc.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, debugName );

    VkMemoryRequirements memoryRequirements = {};
    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, outFramebufferDesc.image, &memoryRequirements );

    VkMemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = nullptr;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = GetMemoryType( memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    result = vkAllocateMemory( GfxDeviceGlobal::device, &memoryAllocateInfo, nullptr, &outFramebufferDesc.deviceMemory );
    if (result != VK_SUCCESS)
    {
        System::Print( "Failed to create a framebuffer!\n" );
        return false;
    }

    result = vkBindImageMemory( GfxDeviceGlobal::device, outFramebufferDesc.image, outFramebufferDesc.deviceMemory, 0 );
    if (result != VK_SUCCESS)
    {
        System::Print( "Failed to create a framebuffer!\n" );
        return false;
    }

    VkImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = nullptr;
    imageViewCreateInfo.flags = 0;
    imageViewCreateInfo.image = outFramebufferDesc.image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = imageCreateInfo.format;
    imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    result = vkCreateImageView( GfxDeviceGlobal::device, &imageViewCreateInfo, nullptr, &outFramebufferDesc.imageView );
    if (result != VK_SUCCESS)
    {
        System::Print( "Failed to create a framebuffer!\n" );
        return false;
    }

    // Depth / Stencil
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_D32_SFLOAT;
    imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    result = vkCreateImage( GfxDeviceGlobal::device, &imageCreateInfo, nullptr, &outFramebufferDesc.depthStencilImage );
    if (result != VK_SUCCESS)
    {
        System::Print( "Failed to create a framebuffer!\n" );
        return false;
    }
    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, outFramebufferDesc.depthStencilImage, &memoryRequirements );

    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = GetMemoryType( memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    result = vkAllocateMemory( GfxDeviceGlobal::device, &memoryAllocateInfo, nullptr, &outFramebufferDesc.depthStencilDeviceMemory );
    if (result != VK_SUCCESS)
    {
        System::Print( "Failed to create a framebuffer!\n" );
        return false;
    }

    result = vkBindImageMemory( GfxDeviceGlobal::device, outFramebufferDesc.depthStencilImage, outFramebufferDesc.depthStencilDeviceMemory, 0 );
    if (result != VK_SUCCESS)
    {
        System::Print( "Failed to create a framebuffer!\n" );
        return false;
    }

    imageViewCreateInfo.image = outFramebufferDesc.depthStencilImage;
    imageViewCreateInfo.format = imageCreateInfo.format;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    result = vkCreateImageView( GfxDeviceGlobal::device, &imageViewCreateInfo, nullptr, &outFramebufferDesc.depthStencilImageView );
    if (result != VK_SUCCESS)
    {
        System::Print( "Failed to create a framebuffer!\n" );
        return false;
    }

    // Renderpass
    //const std::uint32_t totalAttachments = 2;
    VkAttachmentDescription attachmentDescs[ 2 ] = {};
    VkAttachmentReference attachmentReferences[ 2 ] = {};
    attachmentReferences[ 0 ].attachment = 0;
    attachmentReferences[ 0 ].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentReferences[ 1 ].attachment = 1;
    attachmentReferences[ 1 ].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attachmentDescs[ 0 ].format = VK_FORMAT_R8G8B8A8_SRGB;
    attachmentDescs[ 0 ].samples = imageCreateInfo.samples;
    attachmentDescs[ 0 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescs[ 0 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescs[ 0 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescs[ 0 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescs[ 0 ].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentDescs[ 0 ].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentDescs[ 0 ].flags = 0;

    attachmentDescs[ 1 ].format = VK_FORMAT_D32_SFLOAT;
    attachmentDescs[ 1 ].samples = imageCreateInfo.samples;
    attachmentDescs[ 1 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescs[ 1 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescs[ 1 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescs[ 1 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescs[ 1 ].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachmentDescs[ 1 ].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachmentDescs[ 1 ].flags = 0;

    VkSubpassDescription subPassCreateInfo = {};
    subPassCreateInfo.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPassCreateInfo.flags = 0;
    subPassCreateInfo.inputAttachmentCount = 0;
    subPassCreateInfo.pInputAttachments = nullptr;
    subPassCreateInfo.colorAttachmentCount = 1;
    subPassCreateInfo.pColorAttachments = &attachmentReferences[ 0 ];
    subPassCreateInfo.pResolveAttachments = nullptr;
    subPassCreateInfo.pDepthStencilAttachment = &attachmentReferences[ 1 ];
    subPassCreateInfo.preserveAttachmentCount = 0;
    subPassCreateInfo.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = 2;
    renderPassCreateInfo.pAttachments = &attachmentDescs[ 0 ];
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subPassCreateInfo;
    renderPassCreateInfo.dependencyCount = 0;
    renderPassCreateInfo.pDependencies = nullptr;

    result = vkCreateRenderPass( GfxDeviceGlobal::device, &renderPassCreateInfo, nullptr, &outFramebufferDesc.renderPass );
    if (result != VK_SUCCESS)
    {
        System::Print( "Failed to create a framebuffer!\n" );
        return false;
    }

    VkImageView attachments[ 2 ] = { outFramebufferDesc.imageView, outFramebufferDesc.depthStencilImageView };
    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.pNext = nullptr;
    framebufferCreateInfo.renderPass = outFramebufferDesc.renderPass;
    framebufferCreateInfo.attachmentCount = 2;
    framebufferCreateInfo.pAttachments = &attachments[ 0 ];
    framebufferCreateInfo.width = width;
    framebufferCreateInfo.height = height;
    framebufferCreateInfo.layers = 1;
    result = vkCreateFramebuffer( GfxDeviceGlobal::device, &framebufferCreateInfo, nullptr, &outFramebufferDesc.framebuffer );
    if (result != VK_SUCCESS)
    {
        System::Print( "Failed to create a framebuffer!\n" );
        return false;
    }

    outFramebufferDesc.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    outFramebufferDesc.depthStencilImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    return true;
}

void SetupDistortion()
{
    const short lensGridSegmentCountH = 43;
    const short lensGridSegmentCountV = 43;

    const float w = 1.0f / float( lensGridSegmentCountH - 1 );
    const float h = 1.0f / float( lensGridSegmentCountV - 1 );

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
            ae3d::System::Assert( success, "Distortion computation failed" );
            
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
            ae3d::System::Assert( success, "Distortion computation failed" );

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

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.bindingCount = 3;
    descriptorSetLayoutCreateInfo.pBindings = &layoutBindings[ 0 ];
    VkResult err = vkCreateDescriptorSetLayout( GfxDeviceGlobal::device, &descriptorSetLayoutCreateInfo, nullptr, &Global::descriptorSetLayout );
    AE3D_CHECK_VULKAN( err, "Unable to create descriptor set layout" );

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
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

    bool res = CreateFrameBuffer( Global::width, Global::height, Global::leftEyeDesc, "left eye" );
    res = CreateFrameBuffer( Global::width, Global::height, Global::rightEyeDesc, "right eye" );

    Global::lensDistort.LoadSPIRV( ae3d::FileSystem::FileContents( "vr_companion_vert.spv" ), ae3d::FileSystem::FileContents( "vr_companion_vert.spv" ) );
    SetupDescriptors();
    SetupDistortion();

    vr::HmdMatrix34_t matEye = Global::hmd->GetEyeToHeadTransform( vr::Eye_Left );
    ConvertSteamVRMatrixToMatrix4( matEye, Global::eyePosLeft );
        
    matEye = Global::hmd->GetEyeToHeadTransform( vr::Eye_Right );
    ConvertSteamVRMatrixToMatrix4( matEye, Global::eyePosRight );

    if (!vr::VRCompositor())
    {
        System::Assert( false, "Unable to init VR compositor!\n" );
        return;
    }
}

void ae3d::VR::Deinit()
{
    if (Global::hmd)
    {
        vkDestroyImage( GfxDeviceGlobal::device, Global::leftEyeDesc.image, nullptr );
        vkDestroyImageView( GfxDeviceGlobal::device, Global::leftEyeDesc.imageView, nullptr );
        vkFreeMemory( GfxDeviceGlobal::device, Global::leftEyeDesc.deviceMemory, nullptr );

        vkDestroyImage( GfxDeviceGlobal::device, Global::rightEyeDesc.image, nullptr );
        vkDestroyImageView( GfxDeviceGlobal::device, Global::rightEyeDesc.imageView, nullptr );
        vkFreeMemory( GfxDeviceGlobal::device, Global::rightEyeDesc.deviceMemory, nullptr );

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
    vulkanData.m_nImage = (uint64_t)Global::leftEyeDesc.image;
    vulkanData.m_pDevice = GfxDeviceGlobal::device;
    vulkanData.m_pPhysicalDevice = GfxDeviceGlobal::physicalDevice;
    vulkanData.m_pInstance = GfxDeviceGlobal::instance;
    vulkanData.m_pQueue = GfxDeviceGlobal::graphicsQueue;
    vulkanData.m_nQueueFamilyIndex = GfxDeviceGlobal::graphicsQueueIndex;

    vulkanData.m_nWidth = Global::leftEyeDesc.width;
    vulkanData.m_nHeight = Global::leftEyeDesc.height;
    vulkanData.m_nFormat = VK_FORMAT_R8G8B8A8_SRGB;
    vulkanData.m_nSampleCount = 1;

    vr::VRTextureBounds_t bounds;
    bounds.uMin = 0.0f;
    bounds.uMax = 1.0f;
    bounds.vMin = 0.0f;
    bounds.vMax = 1.0f;

    vr::Texture_t texture = { &vulkanData, vr::TextureType_Vulkan, vr::ColorSpace_Auto };
    vr::EVRCompositorError submitResult = vr::VRCompositor()->Submit( vr::Eye_Left, &texture, &bounds );

    if (submitResult != vr::VRCompositorError_None)
    {
        System::Print( "VR submit for left eye returned error %d\n", submitResult );
    }

    vulkanData.m_nImage = (uint64_t)Global::rightEyeDesc.image;
    vulkanData.m_nWidth = Global::rightEyeDesc.width;
    vulkanData.m_nHeight = Global::rightEyeDesc.height;

    submitResult = vr::VRCompositor()->Submit( vr::Eye_Right, &texture, &bounds );
    
    if (submitResult != vr::VRCompositorError_None)
    {
        System::Print( "VR submit for right eye returned error %d\n", submitResult );
    }

    // FIXME: Maybe this should be moved after present
    vr::VRCompositor()->PostPresentHandoff();
}

void ae3d::VR::SetEye( int eye )
{
    if (!Global::hmd)
    {
        return;
    }

    VRGlobal::eye = eye;

    if (eye == 0)
    {
        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::offscreenCmdBuffer, &cmdBufInfo );
        AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer" );
    }

    VkViewport viewport = { 0.0f, 0.0f, (float)Global::width, (float)Global::height, 0.0f, 1.0f };
    vkCmdSetViewport( GfxDeviceGlobal::offscreenCmdBuffer, 0, 1, &viewport );
    VkRect2D scissor = { 0, 0, Global::width, Global::height };
    vkCmdSetScissor( GfxDeviceGlobal::offscreenCmdBuffer, 0, 1, &scissor );

    FramebufferDesc& fbDesc = eye == 0 ? Global::leftEyeDesc : Global::rightEyeDesc;

    // Transition eye image to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    VkImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    imageMemoryBarrier.oldLayout = eye == 0 ? Global::leftEyeDesc.imageLayout : Global::rightEyeDesc.imageLayout;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageMemoryBarrier.image = fbDesc.image;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    imageMemoryBarrier.srcQueueFamilyIndex = GfxDeviceGlobal::graphicsQueueIndex;
    imageMemoryBarrier.dstQueueFamilyIndex = GfxDeviceGlobal::graphicsQueueIndex;
    vkCmdPipelineBarrier( GfxDeviceGlobal::offscreenCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier );
    fbDesc.imageLayout = imageMemoryBarrier.newLayout;

    // Transition the depth buffer to VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL on first use
    if (fbDesc.depthStencilImageLayout == VK_IMAGE_LAYOUT_UNDEFINED)
    {
        imageMemoryBarrier.image = fbDesc.depthStencilImage;
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        imageMemoryBarrier.oldLayout = fbDesc.depthStencilImageLayout;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        vkCmdPipelineBarrier( GfxDeviceGlobal::offscreenCmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier );
        fbDesc.depthStencilImageLayout = imageMemoryBarrier.newLayout;
    }

    // Start the renderpass
    VkRenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = fbDesc.renderPass;
    renderPassBeginInfo.framebuffer = fbDesc.framebuffer;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = Global::width;
    renderPassBeginInfo.renderArea.extent.height = Global::height;
    renderPassBeginInfo.clearValueCount = 2;
    VkClearValue clearValues[ 2 ];
    clearValues[ 0 ].color.float32[ 0 ] = 0.0f;
    clearValues[ 0 ].color.float32[ 1 ] = 0.0f;
    clearValues[ 0 ].color.float32[ 2 ] = 0.0f;
    clearValues[ 0 ].color.float32[ 3 ] = 1.0f;
    clearValues[ 1 ].depthStencil.depth = 1.0f;
    clearValues[ 1 ].depthStencil.stencil = 0;
    renderPassBeginInfo.pClearValues = &clearValues[ 0 ];
    vkCmdBeginRenderPass( GfxDeviceGlobal::offscreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
}

void ae3d::VR::UnsetEye( int eye )
{
    if (!Global::hmd)
    {
        return;
    }

    vkCmdEndRenderPass( GfxDeviceGlobal::offscreenCmdBuffer );

    FramebufferDesc& fbDesc = eye == 0 ? Global::leftEyeDesc : Global::rightEyeDesc;
    System::Assert( fbDesc.image != VK_NULL_HANDLE, "UnsetEye has bad image!" );

    // Transition eye image to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for display on the companion window
    VkImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.image = fbDesc.image;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    imageMemoryBarrier.srcQueueFamilyIndex = GfxDeviceGlobal::graphicsQueueIndex;
    imageMemoryBarrier.dstQueueFamilyIndex = GfxDeviceGlobal::graphicsQueueIndex;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageMemoryBarrier.oldLayout = fbDesc.imageLayout;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier( GfxDeviceGlobal::offscreenCmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier );
    fbDesc.imageLayout = imageMemoryBarrier.newLayout;

    if (eye == 1)
    {
        vkEndCommandBuffer( GfxDeviceGlobal::offscreenCmdBuffer );
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
    Global::poseClasses.clear();

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
