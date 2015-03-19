#include "System.hpp"
#include <fstream>
#if _MSC_VER
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#endif
#include <cstdarg>

void ae3d::System::EnableWindowsMemleakDetection()
{
#if _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
}

ae3d::System::FileContentsData ae3d::System::FileContents(const char* path)
{
    ae3d::System::FileContentsData outData;

    std::ifstream ifs(path, std::ios::binary);

    outData.data.assign(std::istreambuf_iterator< char >(ifs), std::istreambuf_iterator< char >());
    outData.path = std::string(path);

    return outData;
}

void ae3d::System::Print(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);

    static char msg[256];
#if _MSC_VER
    vsnprintf_s(msg, sizeof(msg), format, ap);
#else
    vsnprintf(msg, sizeof(msg), format, ap);
#endif
    va_end(ap);
    std::printf("%s\n", msg);
#ifdef _MSC_VER
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
#endif
    std::fflush(stdout);
}

void ae3d::System::Assert(bool condition, const char* message)
{
    if (!condition)
    {
        Print("Assertion failed: %s", message);

#ifdef _MSC_VER
        __debugbreak();
#else
        // Linux: raise(SIGTRAP)
        assert(false);
#endif
    }
}
