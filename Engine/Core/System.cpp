#include "System.hpp"
#include <fstream>
#include <list>
#include <vector>
#include <string>
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

extern void nsLog(const char* msg);
extern ae3d::Renderer renderer;
extern ae3d::FileWatcher fileWatcher;

struct PakFile
{
    struct FileEntry
    {
        std::string path;
        std::vector< unsigned char > contents;
    };
    
    std::vector< FileEntry > entries;
    std::string path;
};

namespace Global
{
    std::vector< PakFile > pakFiles;
}

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

void ae3d::System::LoadBuiltinAssets()
{
    renderer.builtinShaders.Load();
}

ae3d::System::FileContentsData ae3d::System::FileContents( const char* path )
{
    ae3d::System::FileContentsData outData;
    outData.path = path == nullptr ? "" : std::string( path );

    for (const auto& pakFile : Global::pakFiles)
    {
        for (const auto& entry : pakFile.entries)
        {
            if (entry.path == std::string( path ))
            {
                outData.data = entry.contents;
                outData.isLoaded = true;
                return outData;
            }
        }
    }
    

    std::ifstream ifs(path, std::ios::binary);

    outData.data.assign(std::istreambuf_iterator< char >(ifs), std::istreambuf_iterator< char >());
    outData.isLoaded = ifs.is_open();

    if (!outData.isLoaded)
    {
        Print( "Could not open %s\n", path );
    }
    
    return outData;
}

void ae3d::System::LoadPakFile( const char* path )
{
    if (path == nullptr)
    {
        Print( "LoadPakFile: path is null\n" );
    }
    
    Global::pakFiles.push_back( PakFile() );
    unsigned entryCount = 0;
    std::ifstream ifs( path );
    if (!ifs.is_open())
    {
        Print( "LoadPakFile: Could not open %s\n", path );
    }
    
    ifs.read( (char*)&entryCount, 4 );
    Global::pakFiles.back().entries.resize( entryCount );
    Global::pakFiles.back().path = path;

    for (unsigned i = 0; i < entryCount; ++i)
    {
        char entryPath[ 128 ];
        ifs.read( entryPath, 128 );
        Global::pakFiles.back().entries[ i ].path = entryPath;
        unsigned entrySize = 0;
        ifs.read( (char*)&entrySize, 4 );
        Global::pakFiles.back().entries[ i ].contents.resize( entrySize );
        ifs.read( (char*)Global::pakFiles.back().entries[ i ].contents.data(), entrySize );
    }
}

void ae3d::System::UnloadPakFile( const char* path )
{
    int i = 0;
    
    for (const PakFile& pakFile : Global::pakFiles)
    {
        if (pakFile.path == std::string( path ))
        {
            Global::pakFiles.erase( std::begin( Global::pakFiles ) + i );
            ++i;
            return;
        }
    }
}

void ae3d::System::Print(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);

    static char msg[512];
#if _MSC_VER
    vsnprintf_s(msg, sizeof(msg), format, ap);
#else
    vsnprintf(msg, sizeof(msg), format, ap);
#endif
    va_end(ap);
    std::printf("%s", msg);
#if _MSC_VER
    OutputDebugStringA(msg);
#endif
#if __APPLE__
    //nsLog(msg);
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
