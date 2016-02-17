#include "GfxDevice.hpp"
#include <array>
#include <vector> 
#include <vulkan/vulkan.h>
#include "System.hpp"
#include "VertexBuffer.hpp"

namespace GfxDeviceGlobal
{
    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
}

namespace ae3d
{
    void CreateRenderer( int samples )
    {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Aether3D";
        appInfo.pEngineName = "Aether3D";
        appInfo.apiVersion = VK_MAKE_VERSION( 1, 0, 2 );

        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pNext = nullptr;
        instanceCreateInfo.pApplicationInfo = &appInfo;

        const char* enabledExtensions[] = { VK_KHR_SURFACE_EXTENSION_NAME };
        instanceCreateInfo.enabledExtensionCount = 1;
        instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions;

        VkResult result = vkCreateInstance( &instanceCreateInfo, nullptr, &GfxDeviceGlobal::instance );

        if (result != VK_SUCCESS)
        {
            System::Print( "Could not create Vulkan instance\n" );
        }

        System::Print( "created a Vulkan instance\n" );

        uint32_t gpuCount;
        result = vkEnumeratePhysicalDevices( GfxDeviceGlobal::instance, &gpuCount, &GfxDeviceGlobal::physicalDevice );

        // Finds graphics queue.
        uint32_t graphicsQueueIndex = 0;
        uint32_t queueCount;
        vkGetPhysicalDeviceQueueFamilyProperties( GfxDeviceGlobal::physicalDevice, &queueCount, nullptr );
        if (queueCount < 1)
        {
            System::Print( "Your system doesn't have physical Vulkan devices.\n" );
        }

        std::vector< VkQueueFamilyProperties > queueProps;
        queueProps.resize( queueCount );
        vkGetPhysicalDeviceQueueFamilyProperties( GfxDeviceGlobal::physicalDevice, &queueCount, queueProps.data() );

        for (graphicsQueueIndex = 0; graphicsQueueIndex < queueCount; graphicsQueueIndex++)
        {
            if (queueProps[ graphicsQueueIndex ].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                break;
            }
        }

        System::Assert( graphicsQueueIndex < queueCount, "graphicsQueueIndex" );

        std::array<float, 1> queuePriorities = { 0.0f };
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = queuePriorities.data();

        const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = nullptr;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.pEnabledFeatures = nullptr;
        deviceCreateInfo.enabledExtensionCount = 1;
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
        result = vkCreateDevice( GfxDeviceGlobal::physicalDevice, &deviceCreateInfo, nullptr, &GfxDeviceGlobal::device );
        
        if (result != VK_SUCCESS)
        {
            System::Print( "Could not create Vulkan device\n" );
        }
    }
}

void ae3d::GfxDevice::SetBackFaceCulling( bool enable )
{

}

void ae3d::GfxDevice::SetClearColor( float red, float green, float blue )
{

}

void ae3d::GfxDevice::ResetFrameStatistics()
{
}

int ae3d::GfxDevice::GetDrawCalls()
{
    return 0;
}

int ae3d::GfxDevice::GetTextureBinds()
{
    return 0;
}

int ae3d::GfxDevice::GetRenderTargetBinds()
{
    return 0;
}

int ae3d::GfxDevice::GetVertexBufferBinds()
{
    return 0;
}

int ae3d::GfxDevice::GetShaderBinds()
{
    return 0;
}

void ae3d::GfxDevice::Init( int width, int height )
{

}

void ae3d::GfxDevice::ClearScreen( unsigned clearFlags )
{

}

void ae3d::GfxDevice::Draw( VertexBuffer& vertexBuffer, int startIndex, int endIndex, Shader& shader, BlendMode blendMode, DepthFunc depthFunc )
{
    ae3d::System::Assert( startIndex > -1 && startIndex <= vertexBuffer.GetFaceCount() / 3, "Invalid vertex buffer draw range in startIndex" );
    ae3d::System::Assert( endIndex > -1 && endIndex >= startIndex && endIndex <= vertexBuffer.GetFaceCount() / 3, "Invalid vertex buffer draw range in endIndex" );
}

void ae3d::GfxDevice::ErrorCheck( const char* info )
{

}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
    vkDestroyDevice( GfxDeviceGlobal::device, nullptr );
    vkDestroyInstance( GfxDeviceGlobal::instance, nullptr );
}

void ae3d::GfxDevice::SetRenderTarget( RenderTexture* target, unsigned cubeMapFace )
{

}

void ae3d::GfxDevice::SetMultiSampling( bool enable )
{
}
