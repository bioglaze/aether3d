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
#include "Matrix.hpp"
#include "Renderer.hpp"
#include "Texture2D.hpp"
#include "Shader.hpp"
#include "Statistics.hpp"

extern ae3d::Renderer renderer;
extern ae3d::FileWatcher fileWatcher;
void PlatformInitGamePad();

#if _MSC_VER
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

void ae3d::System::InitGfxDeviceForEditor( int width, int height )
{
    GfxDevice::Init( width, height );
}

#if RENDERER_METAL
void ae3d::System::InitMetal( id <MTLDevice> device, MTKView* view, int sampleCount )
{
    GfxDevice::InitMetal( device, view, sampleCount );
}

void ae3d::System::SetCurrentDrawableMetal( id <CAMetalDrawable> drawable, MTLRenderPassDescriptor* renderPass )
{
    GfxDevice::SetCurrentDrawableMetal( drawable, renderPass );
}

void ae3d::System::BeginFrame()
{
    GfxDevice::BeginFrame();
}

void ae3d::System::EndFrame()
{
    GfxDevice::PresentDrawable();
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
    //_crtBreakAlloc = 48;    // Break at allocation number 48.
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
    renderer.GenerateQuadBuffer();
    renderer.GenerateSkybox();
    renderer.GenerateTextures();
    GfxDevice::ErrorCheck( "Builtin shaders load end" );
}

void ae3d::System::Print( const char* format, ... )
{
    va_list ap;
    va_start(ap, format);

    static char msg[ 1024 ];
#if _MSC_VER
    vsnprintf_s( msg, sizeof(msg), format, ap );
#else
    vsnprintf( msg, sizeof(msg), format, ap );
#endif
    va_end(ap);
    std::printf( "%s", msg );
#if _MSC_VER
    OutputDebugStringA( &msg[ 0 ] );
#endif
    std::fflush( stdout );
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

void ae3d::System::Draw( Texture2D* texture, float x, float y, float xSize, float ySize, float xScreenSize, float yScreenSize )
{
    Matrix44 proj;
    proj.MakeProjection( 0, xScreenSize, yScreenSize, 0, -1, 1 );
    
    Matrix44 translate;
    translate.Translate( Vec3( x, y, 0 ) );
    
    Matrix44 scale;
    scale.Scale( xSize, ySize, 1 );
    
    Matrix44 mvp;
    Matrix44::Multiply( scale, translate, scale );
    Matrix44::Multiply( scale, proj, mvp );
    
    renderer.builtinShaders.spriteRendererShader.Use();
    renderer.builtinShaders.spriteRendererShader.SetMatrix( "_ProjectionModelMatrix", &mvp.m[ 0 ] );
    renderer.builtinShaders.spriteRendererShader.SetTexture( "textureMap", texture, 1 );
    
    GfxDevice::Draw( renderer.GetQuadBuffer(), 0, 2, renderer.builtinShaders.spriteRendererShader, GfxDevice::BlendMode::AlphaBlend, GfxDevice::DepthFunc::NoneWriteOff, GfxDevice::CullMode::Off );
}

void ae3d::System::ReloadChangedAssets()
{
    fileWatcher.Poll();
}

int ae3d::System::Statistics::GetDrawCallCount()
{
    return ::Statistics::GetDrawCalls();
}

int ae3d::System::Statistics::GetVertexBufferBindCount()
{
    return ::Statistics::GetVertexBufferBinds();
}

int ae3d::System::Statistics::GetTextureBindCount()
{
    return ::Statistics::GetTextureBinds();
}

int ae3d::System::Statistics::GetRenderTargetBindCount()
{
    return ::Statistics::GetRenderTargetBinds();
}

int ae3d::System::Statistics::GetShaderBindCount()
{
    return ::Statistics::GetShaderBinds();
}

void ae3d::System::Statistics::GetGpuMemoryUsage( unsigned& outUsedMBytes, unsigned& outBudgetMBytes )
{
    GfxDevice::GetGpuMemoryUsage( outUsedMBytes, outBudgetMBytes );
}

int ae3d::System::Statistics::GetBarrierCallCount()
{
    return ::Statistics::GetBarrierCalls();
}

int ae3d::System::Statistics::GetFenceCallCount()
{
    return ::Statistics::GetFenceCalls();
}
