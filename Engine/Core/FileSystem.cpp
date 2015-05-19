#include "FileSystem.hpp"
#include "System.hpp"
#include <fstream>
#include <vector>

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

ae3d::FileSystem::FileContentsData ae3d::FileSystem::FileContents(const char* path)
{
    ae3d::FileSystem::FileContentsData outData;
    outData.path = path == nullptr ? "" : std::string(path);

    for (const auto& pakFile : Global::pakFiles)
    {
        for (const auto& entry : pakFile.entries)
        {
            if (entry.path == std::string(path))
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
        System::Print("Could not open %s\n", path);
    }

    return outData;
}

void ae3d::FileSystem::LoadPakFile(const char* path)
{
    if (path == nullptr)
    {
        System::Print("LoadPakFile: path is null\n");
    }

    Global::pakFiles.push_back(PakFile());
    unsigned entryCount = 0;
    std::ifstream ifs(path);
    if (!ifs.is_open())
    {
        System::Print("LoadPakFile: Could not open %s\n", path);
    }

    ifs.read((char*)&entryCount, 4);
    Global::pakFiles.back().entries.resize(entryCount);
    Global::pakFiles.back().path = path;

    for (unsigned i = 0; i < entryCount; ++i)
    {
        char entryPath[128];
        ifs.read(entryPath, 128);
        Global::pakFiles.back().entries[i].path = entryPath;
        unsigned entrySize = 0;
        ifs.read((char*)&entrySize, 4);
        Global::pakFiles.back().entries[i].contents.resize(entrySize);
        ifs.read((char*)Global::pakFiles.back().entries[i].contents.data(), entrySize);
    }
}

void ae3d::FileSystem::UnloadPakFile(const char* path)
{
    int i = 0;

    for (const PakFile& pakFile : Global::pakFiles)
    {
        if (pakFile.path == std::string(path))
        {
            Global::pakFiles.erase(std::begin(Global::pakFiles) + i);
            ++i;
            return;
        }
    }
}
