#ifndef SYSTEM_H
#define SYSTEM_H

#include <string>
#include <vector>

namespace ae3d
{
    namespace System
    {
        struct FileContentsData
        {
            std::vector<char> data;
            std::string path;
        };

        /** Releases all resources allocated by the engine. Call when exiting. */
        void Deinit();

        /** Enables memory leak detection on DEBUG builds on Visual Studio. */
        void EnableWindowsMemleakDetection();

        /**
          Reads file contents.
          
          \param path Path.
          */
        FileContentsData FileContents( const char* path );

        /**
          \param condition Condition that causes the assert to fire when true.
          \param message Assert message to be displayed when condition is true.
        */
        void Assert(bool condition, const char* message);

        /*
        Formats a message and prints it into stdout.
        Example: Log::PrintFormat( "My name is %s", name.c_str() ); // where name is a std::string.
        Formatting examples:
        %d: integer
        %s: C-style null-terminated string
        %f: float
        */
        void Print(const char* format, ...);
    }
}
#endif
