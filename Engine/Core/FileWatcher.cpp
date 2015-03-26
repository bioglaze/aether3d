#include "FileWatcher.hpp"
#include <ctime>
#include <sys/stat.h>
#include <locale.h>

ae3d::FileWatcher fileWatcher;

void ae3d::FileWatcher::AddFile( const std::string& path, std::function<void(const std::string&)> updateFunc )
{
    pathToEntry[ path ] = Entry();
    pathToEntry[ path ].path = path;
    pathToEntry[ path ].updateFunc = updateFunc;

    struct stat inode;

    if (stat( path.c_str(), &inode ) != -1)
    {
#if _MSC_VER
        tm timeinfo;
        localtime_s( &timeinfo, &inode.st_mtime );
        tm* timeinfo2 = &timeinfo;
#else
        tm* timeinfo2 = localtime( &inode.st_mtime );
#endif
        pathToEntry[ path ].hour = timeinfo2->tm_hour;
        pathToEntry[ path ].minute = timeinfo2->tm_min;
        pathToEntry[ path ].second = timeinfo2->tm_sec;
    }
}

void ae3d::FileWatcher::Poll()
{
    struct stat inode;

    for (auto& entry : pathToEntry)
    {
        if (stat( entry.second.path.c_str(), &inode ) != -1)
        {
#if _MSC_VER
            tm timeinfo;
            localtime_s( &timeinfo, &inode.st_mtime );
            tm* timeinfo2 = &timeinfo;
#else
            tm* timeinfo2 = localtime( &inode.st_mtime );
#endif

            if (timeinfo2->tm_hour != entry.second.hour || timeinfo2->tm_min != entry.second.minute || timeinfo2->tm_sec != entry.second.second)
            {
                entry.second.updateFunc( entry.second.path );
                entry.second.hour = timeinfo2->tm_hour;
                entry.second.minute = timeinfo2->tm_min;
                entry.second.second = timeinfo2->tm_sec;
            }
        }
    }
}
