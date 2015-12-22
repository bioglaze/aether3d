#include "System.hpp"
#if _MSC_VER
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#endif
#include <cstdarg>
#include <cassert>
#include "AudioSystem.hpp"
#include "GfxDevice.hpp"
#include "FileWatcher.hpp"
#include "Renderer.hpp"

extern ae3d::Renderer renderer;
extern ae3d::FileWatcher fileWatcher;
void PlatformInitGamePad();

void ae3d::System::InitGfxDeviceForEditor( int width, int height )
{
    GfxDevice::Init( width, height );
}

#if AETHER3D_METAL
void ae3d::System::InitMetal( id <MTLDevice> device, MTKView* view)
{
    GfxDevice::InitMetal( device, view );
}

void ae3d::System::SetCurrentDrawableMetal( id <CAMetalDrawable> drawable, MTLRenderPassDescriptor* renderPass )
{
    GfxDevice::SetCurrentDrawableMetal( drawable, renderPass );
}

void ae3d::System::EndFrame()
{
    GfxDevice::FlipBuffers();
}

void ae3d::System::BeginFrame()
{
    GfxDevice::BeginFrame();
}

#endif

void ae3d::System::Deinit()
{
    GfxDevice::ReleaseGPUObjects();
    AudioSystem::Deinit();
}

void ae3d::System::EnableWindowsMemleakDetection()
{
#if _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
}

void ae3d::System::InitAudio()
{
    AudioSystem::Init();
}

void ae3d::System::InitGamePad()
{
    PlatformInitGamePad();
}

void ae3d::System::LoadBuiltinAssets()
{
    renderer.builtinShaders.Load();
}

void ae3d::System::Print(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);

    static char msg[ 1024 ];
#if _MSC_VER
    vsnprintf_s(msg, sizeof(msg), format, ap);
#else
    vsnprintf(msg, sizeof(msg), format, ap);
#endif
    va_end(ap);
    std::printf("%s", msg);
#if _MSC_VER
    OutputDebugStringA( &msg[ 0 ] );
#endif
    std::fflush(stdout);
}

void ae3d::System::Assert(bool condition, const char* message)
{
    if (!condition)
    {
        Print("Assertion failed: %s\n", message);

#ifdef _MSC_VER
        __debugbreak();
#else
        assert(false);
#endif
    }
}

void ae3d::System::ReloadChangedAssets()
{
    fileWatcher.Poll();
}

int ae3d::System::Statistics::GetDrawCallCount()
{
    return GfxDevice::GetDrawCalls();
}

int ae3d::System::Statistics::GetVertexBufferBindCount()
{
    return GfxDevice::GetVertexBufferBinds();
}

int ae3d::System::Statistics::GetTextureBindCount()
{
    return GfxDevice::GetTextureBinds();
}

int ae3d::System::Statistics::GetRenderTargetBindCount()
{
    return GfxDevice::GetRenderTargetBinds();
}

int ae3d::System::Statistics::GetShaderBindCount()
{
    return GfxDevice::GetShaderBinds();
}
