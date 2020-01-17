#pragma once

#include <string>
#include <vector>

namespace ae3d
{
    namespace FileSystem
    {
        /** File contents. */
        struct FileContentsData
        {
            /// File content bytes.
            std::vector<unsigned char> data;
            /// File path.
            std::string path;

#if defined __APPLE__
            /// Original path. Path without bundle.
            std::string pathWithoutBundle;
#endif            
            /// True if data has been loaded from path.
            bool isLoaded = false;
        };

        /**
        Reads file contents.

        \param path Path.
        */
        FileContentsData FileContents( const char* path );

        /// \param path .pak file path. After this call FileContents() searches first in all loaded .pak files and if the file is not found, it's loaded without .pak file.
        void LoadPakFile( const char* path );

        /// \param path .pak file. If it was loaded, it's unloaded and FileContents() does not search files inside it.
        void UnloadPakFile( const char* path );
    }
}
