#ifndef FILEWATCHER_H
#define FILEWATCHER_H

#include <string>
#include <functional>
#include <map>

namespace ae3d
{
    /** Keeps track of files and calls updateFunc when they have changed on disk. This enables asset hotloading. */
    class FileWatcher
    {
    public:
        void AddFile( const std::string& path, std::function<void(const std::string&)> updateFunc );
        // Scans for all watched file modification timestamps and calls updateFunc if they have been updated.
        void Poll();

    private:
        struct Entry
        {
            int hour = 0;
            int minute = 0;
            int second = 0;
            std::string path;
            std::function<void(const std::string&)> updateFunc;
        };
        
        std::map< std::string, Entry > pathToEntry;
    };    
}

#endif
