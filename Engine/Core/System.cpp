// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "System.hpp"
#if _MSC_VER
#define _AMD64_
#include <debugapi.h>
#endif
#if VK_USE_PLATFORM_ANDROID_KHR
#include <android/log.h>
#endif
#include <stdarg.h>
#include <assert.h>
#include "AudioSystem.hpp"
#include "GfxDevice.hpp"
#include "FileWatcher.hpp"
#include "Matrix.hpp"
#include "Renderer.hpp"
#include "Shader.hpp"
#include "Statistics.hpp"
#include "Texture2D.hpp"
#include "Vec3.hpp"

extern ae3d::Renderer renderer;
extern ae3d::FileWatcher fileWatcher;

namespace GfxDeviceGlobal
{
    extern PerObjectUboStruct perObjectUboStruct;
}

void PlatformInitGamePad();

using namespace ae3d;

namespace MathUtil
{
    bool IsPowerOfTwo( unsigned i );
    int Min( int x, int y );
    int Max( int x, int y );
}

#if _MSC_VER
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#if RENDERER_METAL
void ae3d::System::InitMetal( id <MTLDevice> device, MTKView* view, int sampleCount, int uiVBSize, int uiIBSize )
{
    GfxDevice::InitMetal( device, view, sampleCount, uiVBSize, uiIBSize );
}

void ae3d::System::SetCurrentDrawableMetal( MTKView* view )
{
    GfxDevice::SetCurrentDrawableMetal( view );
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

void ae3d::System::MapUIVertexBuffer( int vertexSize, int indexSize, void** outMappedVertices, void** outMappedIndices )
{
    GfxDevice::MapUIVertexBuffer( vertexSize, indexSize, outMappedVertices, outMappedIndices );
}

void ae3d::System::UnmapUIVertexBuffer()
{
    GfxDevice::UnmapUIVertexBuffer();
}

void ae3d::System::DrawUI( int scX, int scY, int scWidth, int scHeight, int elemCount, Texture2D* texture, int offset, int windowWidth, int windowHeight )
{
    float ortho[ 4 ][ 4 ] = {
        { 2, 0, 0, 0 },
        { 0,-2, 0, 0 },
        { 0, 0,-1, 0 },
        { -1,1, 0, 1 },
    };
    ortho[ 0 ][ 0 ] /= (float)windowWidth;
    ortho[ 1 ][ 1 ] /= (float)windowHeight;

    renderer.builtinShaders.uiShader.Use();
    renderer.builtinShaders.uiShader.SetTexture( texture, 0 );
    GfxDeviceGlobal::perObjectUboStruct.localToClip.InitFrom( &ortho[ 0 ][ 0 ] );
    GfxDeviceGlobal::perObjectUboStruct.lightColor = Vec4( 1, 1, 1, 1 );

    int viewport[ 4 ];
    viewport[ 0 ] = 0;
    viewport[ 1 ] = 0;
    viewport[ 2 ] = windowWidth;
    viewport[ 3 ] = windowHeight;
    GfxDevice::SetViewport( viewport );

    GfxDevice::DrawUI( scX, scY, scWidth, scHeight, elemCount, offset );
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
}

void ae3d::System::Print( const char* format, ... )
{
    va_list ap;
    va_start(ap, format);

    static char msg[ 2048 ];
#if _MSC_VER
    vsnprintf_s( msg, sizeof(msg), format, ap );
#else
    vsnprintf( msg, sizeof(msg), format, ap );
#endif
    va_end(ap);
    std::printf( "%s", msg );
#if VK_USE_PLATFORM_ANDROID_KHR
    __android_log_print( ANDROID_LOG_INFO, "Aether3D", "%s", msg );
#endif
#if _MSC_VER
    OutputDebugStringA( &msg[ 0 ] );
#endif
    std::fflush( stdout );
}

void ae3d::System::Assert( bool condition, const char* message )
{
//#if DEBUG
#if 1
    if (!condition)
    {
        Print("Assertion failed: %s\n", message);

#ifdef _MSC_VER
        __debugbreak();
#else
        assert( false );
#endif
    }
#endif
}

void ae3d::System::Draw( TextureBase* texture, int x, int y, int xSize, int ySize, int xScreenSize, int yScreenSize, const Vec4& tintColor, BlendMode blendMode )
{
    Matrix44 proj;
    proj.MakeProjection( 0, (float)xScreenSize, (float)yScreenSize, 0, -1, 1 );
    
    Matrix44 translate;
    translate.SetTranslation( Vec3( (float)x, (float)y, 0 ) );
    
    Matrix44 scale;
    scale.Scale( (float)xSize, (float)ySize, 1 );
    
    Matrix44 mvp;
    Matrix44::Multiply( scale, translate, scale );
    Matrix44::Multiply( scale, proj, mvp );
    
    renderer.builtinShaders.spriteRendererShader.Use();

    if (texture->IsCube() && !texture->IsRenderTexture())
    {
        renderer.builtinShaders.spriteRendererShader.SetTexture( (TextureCube*)texture, 0 );
    }
    else if (!texture->IsCube() && !texture->IsRenderTexture())
    {
        renderer.builtinShaders.spriteRendererShader.SetTexture( (Texture2D*)texture, 0 );
    }
    else if (texture->IsRenderTexture())
    {
        renderer.builtinShaders.spriteRendererShader.SetRenderTexture( (RenderTexture*)texture, 0 );
    }
    
    GfxDeviceGlobal::perObjectUboStruct.localToClip = mvp;
    GfxDeviceGlobal::perObjectUboStruct.lightColor = tintColor;
    
    int viewport[ 4 ];
    viewport[ 0 ] = 0;
    viewport[ 1 ] = 0;
    viewport[ 2 ] = xScreenSize;
    viewport[ 3 ] = yScreenSize;
    GfxDevice::SetViewport( viewport );

    GfxDevice::BlendMode gfxBlendMode = GfxDevice::BlendMode::Off;
    
    if (blendMode == BlendMode::Alpha)
    {
        gfxBlendMode = GfxDevice::BlendMode::AlphaBlend;
    }
    else if (blendMode == BlendMode::Additive)
    {
        gfxBlendMode = GfxDevice::BlendMode::Additive;
    }

    GfxDevice::Draw( renderer.GetQuadBuffer(), 0, 2, renderer.builtinShaders.spriteRendererShader, gfxBlendMode,
                     GfxDevice::DepthFunc::NoneWriteOff, GfxDevice::CullMode::Off, GfxDevice::FillMode::Solid, GfxDevice::PrimitiveTopology::Triangles );
}

int ae3d::System::CreateLineBuffer( const Vec3* lines, int lineCount, const Vec3& color )
{    
    return GfxDevice::CreateLineBuffer( lines, lineCount, color );
}

void ae3d::System::UpdateLineBuffer( int lineHandle, const Vec3* lines, int lineCount, const Vec3& color )
{
    GfxDevice::UpdateLineBuffer( lineHandle, lines, lineCount, color );
}

void ae3d::System::DrawLines( int handle, const Matrix44& view, const Matrix44& projection, int xScreenSize, int yScreenSize )
{
    Matrix44 viewProjection;
    Matrix44::Multiply( view, projection, viewProjection );
    renderer.builtinShaders.spriteRendererShader.Use();
    renderer.builtinShaders.spriteRendererShader.SetTexture( Texture2D::GetDefaultTexture(), 0 );
    GfxDeviceGlobal::perObjectUboStruct.lightColor = Vec4( 1, 1, 1, 1 );
    GfxDeviceGlobal::perObjectUboStruct.localToClip = viewProjection;

    int viewport[ 4 ];
    viewport[ 0 ] = 0;
    viewport[ 1 ] = 0;
    viewport[ 2 ] = xScreenSize;
    viewport[ 3 ] = yScreenSize;
    GfxDevice::SetViewport( viewport );

    GfxDevice::DrawLines( handle, renderer.builtinShaders.spriteRendererShader );
}

void ae3d::System::ReloadChangedAssets()
{
    fileWatcher.Poll();
}

void ae3d::System::Statistics::SetBloomTime( float cpuMs, float gpuMs )
{
    ::Statistics::SetBloomTime( cpuMs, gpuMs );
}

int ae3d::System::Statistics::GetDrawCallCount()
{
    return ::Statistics::GetDrawCalls();
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

void ae3d::System::RunUnitTests()
{
    const bool isPowerOfTwo2 = MathUtil::IsPowerOfTwo( 2 );
    Assert( isPowerOfTwo2, "IsPowerOfTwo failed" );

    const bool isPowerOfTwo0 = MathUtil::IsPowerOfTwo( 0 );
    Assert( isPowerOfTwo0, "IsPowerOfTwo failed" );

    const int min1 = MathUtil::Min( 1, 2 );
    Assert( min1 == 1, "Min failed" );

    const int min2 = MathUtil::Min( 2, 1 );
    Assert( min2 == 1, "Min failed" );

    const int min3 = MathUtil::Min( 1, 1 );
    Assert( min3 == 1, "Min failed" );

    const int min4 = MathUtil::Min( -1, 1 );
    Assert( min4 == -1, "Min failed" );

    const int min5 = MathUtil::Min( -1, -2 );
    Assert( min5 == -2, "Min failed" );

    const int max1 = MathUtil::Max( 1, 2 );
    Assert( max1 == 2, "Max failed" );

    const int max2 = MathUtil::Max( -1, 2 );
    Assert( max2 == 2, "Max failed" );
}

#if !AE3D_OPENVR
namespace ae3d
{
    class GameObject;

    namespace VR
    {
        void Init()
        {
        }
        
        void Deinit()
        {
        }
        
        void StartTracking( int, int )
        {
        }
        
        void RecenterTracking()
        {
        }

        Vec3 GetLeftHandPosition()
        {
            return Vec3( -1, 0, 0 );
        }

        Vec3 GetRightHandPosition()
        {
            return Vec3( 1, 0, 0 );
        }

        void CalcCameraForEye( class GameObject& /*camera*/, float /*yawDegrees*/, int /*eye*/ )
        {
        }
    }
}
#endif
