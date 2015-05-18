/**
  Combines files listed in input text file into one.
  
  Usage: CombineFiles input.txt output

  Input file contains one path per line.
  
  Output file's first 4 bytes is the block count. After it blocks are followed where data is ordered like below:
  offset      data
      0       file name
    128       file length
    132       file contents
*/
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <string>
#include <vector>

struct FileMetaBlock
{
    std::string path;
    unsigned dataSize;
};

int main( int argCount, char* args[] )
{
    if (argCount != 3)
    {
        std::cout << "Usage: CombineFiles indexFile.txt outputfile" << std::endl;
        return 1;
    }

    std::ifstream fileListFile( args[ 1 ] );
    if (!fileListFile.is_open())
    {
        std::cout << "Could not open " << args[ 1 ] << std::endl;
        return 1;
    }

    std::list< FileMetaBlock > fileList;
    std::string line;
    const unsigned MaxPathLength = 128;

    while (std::getline( fileListFile, line ))
    {
        fileList.push_back( FileMetaBlock() );
        fileList.back().path = line;
        
        if (line.length() > MaxPathLength)
        {
            std::cout << "Too long path in " << line << ". Max length is " << MaxPathLength << std::endl;
            return 1;
        }
    }

    // 1st pass: Determine file sizes.    
    for (auto& file : fileList)
    {
        std::ifstream ifs( file.path );
        ifs.seekg( 0, std::ios::end );
        file.dataSize = static_cast< unsigned >( ifs.tellg() );
    }
    
    std::ofstream ofs( args[ 2 ], std::ios::out | std::ios::binary );
    const unsigned listLength = static_cast< unsigned >( fileList.size() );
    ofs.write( (char*)&listLength, 4 );

    for (const auto& file : fileList)
    {
        char path[ MaxPathLength ] = { 0 };
        std::strcpy( path, file.path.c_str() );
        ofs.write( (char*)path, MaxPathLength );
        ofs.write( (char*)&file.dataSize, 4 );
        std::vector< unsigned char > data;
        std::ifstream ifs( file.path );
        data.assign( std::istreambuf_iterator< char >( ifs ), std::istreambuf_iterator< char >() );
        ofs.write( (char*)data.data(), data.size() );
    }

    return 0;
}
