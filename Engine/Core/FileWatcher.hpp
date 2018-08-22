#pragma once

#include <string>
#include <map>

namespace ae3d
{
    /** Keeps track of files and calls updateFunc when they have changed on disk. This enables asset hotloading. */
    class FileWatcher
    {
    public:
        void AddFile( const std::string& path, void(*updateFunc)(const std::string&)  );
        // Scans for all watched file modification timestamps and calls updateFunc if they have been updated.
        void Poll();

    private:
        struct Entry
        {
            int hour = 0;
            int minute = 0;
            int second = 0;
            std::string path;
            void(*updateFunc)(const std::string&) = nullptr;
        };
        
        std::map< std::string, Entry > pathToEntry;
    };    
}
