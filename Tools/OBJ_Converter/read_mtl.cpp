/**
   Reads materials from .obj's .mtl file and outputs them in
   a format that Aether3D's scene file can use.

   If the given file is a .obj, material names are associated with mesh names.
   If the given file is a .mtl, materials are generated.
 */
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

void ReadObj( const std::string& path )
{
    std::ifstream ifs( path );
	std::ofstream ofs( "read_obj_output.txt" );

    while (!ifs.eof())
    {
        std::string line;
        std::stringstream stm;

        getline( ifs, line );
        stm.str( line );

        std::string token;
        stm >> token;

        if (token == "g" || token == "o")
        {
            std::string meshName;
            stm >> meshName;
            std::cout << "mesh_material " << meshName;
			ofs << "mesh_material " << meshName;
        }
        if (token == "usemtl")
        {
            std::string matName;
            stm >> matName;
            std::cout << " " << matName << std::endl;
			ofs << " " << matName << "\n";
        }
    }
}

void ReadMtl( const std::string& path )
{
	std::ofstream ofs( "read_mtl_output.txt" );

    // Generates textures
    {
        std::ifstream ifs( path );

        while (!ifs.eof())
        {
            std::string line;
            std::stringstream stm;

            getline( ifs, line );
            stm.str( line );

            std::string token;
            stm >> token;

            if (token == "map_Kd" || token == "map_bump")
            {
                std::string texPath;
                stm >> texPath;
                std::cout << "texture2d " << texPath << " " << texPath << std::endl;
				ofs << "texture2d " << texPath << " " << texPath << "\n";
            }
        }
    }

    std::ifstream ifs( path );

    while (!ifs.eof())
    {
        std::string line;
        std::stringstream stm;

        getline( ifs, line );
        stm.str( line );

        std::string token;
        stm >> token;

        if (token == "newmtl")
        {
            std::string materialName;
            stm >> materialName;
            std::cout << "material " << materialName << std::endl;
            std::cout << "shaders cook_torrance cook_torrance" << std::endl;
            std::cout << "param_float roughness 0.2" << std::endl;
            std::cout << "param_vec3 materialSpecular 0.2 0.2 0.2" << std::endl;

			ofs << "material " << materialName << std::endl;
			ofs << "shaders cook_torrance cook_torrance" << std::endl;
			ofs << "param_float roughness 0.2" << std::endl;
			ofs << "param_vec3 materialSpecular 0.2 0.2 0.2" << std::endl;
        }
        else if (token == "map_Kd")
        {
            std::string texPath;
            stm >> texPath;
			std::cout << "param_texture diffuseMap " << texPath << std::endl;
			
			ofs << "param_texture diffuseMap " << texPath << std::endl;
        }
        else if (token == "map_bump")
        {
            std::string texPath;
            stm >> texPath;
			std::cout << "param_texture normalMap " << texPath << std::endl;
			
			ofs << "param_texture normalMap " << texPath << std::endl;
        }
    }
}

int main( int argCount, char** argStrings )
{
    if (argCount == 1)
    {
        std::cout << "Usage 1: ./read_mtl file.mtl" << std::endl;
        std::cout << "Usage 2: ./read_mtl file.obj" << std::endl;
        return 0;
    }

    {
        std::ifstream ifs( argStrings[ 1 ] );

        if (!ifs)
        {
            std::cout << "Could not open file." << std::endl;
        }
    }

    if ( std::string( argStrings[ 1 ] ).find( ".obj" ) != std::string::npos)
    {
        ReadObj( std::string( argStrings[ 1 ] ) );
    }
    else if ( std::string( argStrings[ 1 ] ).find( ".mtl" ) != std::string::npos)
    {
        ReadMtl( std::string( argStrings[ 1 ] ) );
    }

	return 0;
}
