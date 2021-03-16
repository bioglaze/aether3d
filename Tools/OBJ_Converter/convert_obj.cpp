// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "../common.hpp"
#include <cmath>
#include <fstream>
#include <map>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Vec3.hpp"

using namespace ae3d;

/**
   Limitations:

   Smoothing groups are not supported.
 */

// Indices are stored in the .obj file relating to the whole object, not
// to a mesh, so we need to convert indices so that they point to submeshes'
// indices.
std::map< int, int > vertGlobalLocal; // (global index, local index)
std::map< int, int > normGlobalLocal; // (global index, local index)
std::map< int, int > tcoordGlobalLocal; // (global index, local index)

bool hasTextureCoords = false;
bool hasVNormals = false;

void ConvertIndices()
{
    for (std::size_t f = 0; f < gMeshes.back().face.size(); ++f)
    {
        auto& face = gMeshes.back().face[ f ];

        if (vertGlobalLocal[ face.vInd[ 0 ] ] > 65535 ||
            vertGlobalLocal[ face.vInd[ 1 ] ] > 65535 ||
            vertGlobalLocal[ face.vInd[ 2 ] ] > 65535)
        {
            std::cerr << "More than 65535 vertices in a mesh, overflow!" << std::endl;
        }
        
        face.vInd[ 0 ] = vertGlobalLocal[ face.vInd[ 0 ] ];
        face.vInd[ 1 ] = vertGlobalLocal[ face.vInd[ 1 ] ];
        face.vInd[ 2 ] = vertGlobalLocal[ face.vInd[ 2 ] ];
        
        if (hasVNormals)
        {
            face.vnInd[ 0 ] = normGlobalLocal[ face.vnInd[ 0 ] ];
            face.vnInd[ 1 ] = normGlobalLocal[ face.vnInd[ 1 ] ];
            face.vnInd[ 2 ] = normGlobalLocal[ face.vnInd[ 2 ] ];
        }
        
        if (hasTextureCoords)
        {
            face.uvInd[ 0 ] = tcoordGlobalLocal[ face.uvInd[ 0 ] ];
            face.uvInd[ 1 ] = tcoordGlobalLocal[ face.uvInd[ 1 ] ];
            face.uvInd[ 2 ] = tcoordGlobalLocal[ face.uvInd[ 2 ] ];
        }
    }
}

/**
   Loads a Wavefront .obj model.

   \todo break into smaller routines.
   \todo don't modify global state.
   \todo Handle relative indexing.
   \param path Path.
 */
void LoadObj( const std::string& path )
{
    std::ifstream ifs( path.c_str() );
    if (!ifs)
    {
        std::cerr << "Couldn't open file " << path << std::endl;
        exit( 1 );
    }
    
    const std::string extension = path.substr( path.length() - 3, path.length() );

    if (extension != "obj" && extension != "OBJ")
    {
        std::cerr << path << " is not .obj!" << std::endl;
        exit( 1 );
    }

    gMeshes.push_back( Mesh() );

    std::string line;
    std::stringstream stm;
    std::string preamble;

    std::vector< Vec3 > vertex;
    std::vector< Vec3 > vnormal;
    std::vector< TexCoord > tcoord;

    // Reads all vertices, normals and texture coordinates to a vector.
    while (!ifs.eof())
    {
        getline( ifs, line );
        stm.clear();
        stm.str( line );
        stm >> preamble;

        if (preamble == "s")
        {
            std::string smoothName;
            stm >> smoothName;
	
            if (smoothName != "off")
            {
                std::cout << "Warning: The file contains smoothing groups. They are not supported by the converter." << std::endl;
            }
        }
        else if (preamble == "v")
        {
            float x, y, z;
            stm >> x >> y >> z;
            vertex.push_back( Vec3( x, y, z ) );
        }
        else if (preamble == "vn")
        {
            float xn, yn, zn;
            stm >> xn >> yn >> zn;
            vnormal.push_back( Vec3( xn, yn, zn ) );
        }
        else if (preamble == "vt")
        {
            float u, v;
            stm >> u >> v;
            tcoord.push_back( TexCoord( u, 1.0f - v ) );
        }
    }

    ifs.close();
    
    if (vnormal.empty())
    {
        std::cout << std::endl << "Warning: The file doesn't contain normals. Generating..." << std::endl;
    }

    ifs.open( path.c_str() );
    ifs.seekg( std::ios_base::beg );

    hasVNormals = !vnormal.empty();

    std::cout << "Reading meshes." << std::endl;
    
    // Reads meshes.
    while (!ifs.eof())
    {
        getline(ifs, line);

        if (ifs.eof())
        {
            break;
        }

        stm.clear();
        stm.str(line);
        stm >> preamble;

        // Object name.
        if (preamble == "o" || preamble == "g")
        {
            // Applies global->local index conversion for previous mesh.
            ConvertIndices();

            if (gMeshes.back().name != "unnamed")
            {
                // Some exporters use 'g' without specifying geometry for it, so remove them.
                if (gMeshes.back().face.empty())
                {
                    gMeshes.erase( std::begin( gMeshes ) + gMeshes.size() - 1 );
                }
                
                gMeshes.push_back( Mesh() );
                hasTextureCoords = false;
            }
            stm >> gMeshes.back().name;

            vertGlobalLocal.clear();
            normGlobalLocal.clear();
            tcoordGlobalLocal.clear();
        }
        else if (preamble == "f")
        {
            int va, vb, vc, vd; // Vertex indices
            int ta; // Texture coordinate indices.
            unsigned char slash; // Slash character between indices.

            Face face;

            // Reads the first vertex.
            stm >> va;
            
            // Relative indexing.
            if (va < 0)
            {
                va = (int)gMeshes.back().vertex.size() - va;
            }
            
            // Didn't find the index in index conversion map, so add it.
            if (vertGlobalLocal.find(va-1) == vertGlobalLocal.end())
            {
                gMeshes.back().vertex.push_back( vertex[va-1] );
                vertGlobalLocal[ va - 1 ] = (int)gMeshes.back().vertex.size() - 1;
            }
            
            face.vInd[ 0 ] = va - 1;
            // Reads '/' if any.
            stm >> slash;

            // There are no texcoords, normals or slashes.
            if (slash != '/')
            {
                stm.unget();

                stm >> vb;
                face.vInd[1] = vb - 1;

                // Didn't find the index in index conversion map, so add it.
                if (vertGlobalLocal.find(vb-1) == vertGlobalLocal.end())
                {
                    gMeshes.back().vertex.push_back( vertex[vb-1] );
                    vertGlobalLocal[ vb - 1 ] = (int)gMeshes.back().vertex.size() - 1;
                }
                
                stm >> vc;
                face.vInd[ 2 ] = vc - 1;

                if (vertGlobalLocal.find( vc - 1) == vertGlobalLocal.end())
                {
                    gMeshes.back().vertex.push_back( vertex[vc-1] );
                    vertGlobalLocal[ vc - 1 ] = (int)gMeshes.back().vertex.size() - 1;
                }
 
                gMeshes.back().face.push_back( face );
                continue;
            }
            
            // Determines the presence of texture coordinates.
            stm >> ta;
            
            // FIXME: should this be inside if() below?
            if (ta < 0)
            {
                ta = (int)gMeshes.back().tcoord.size() - ta;
            }

            if (!stm.fail())
            {
                hasTextureCoords = true;
                // Didn't find the index in index conversion map, so add it.
                if (tcoordGlobalLocal.find(ta-1) == tcoordGlobalLocal.end())
                {
                    gMeshes.back().tcoord.push_back( tcoord[ta-1] );
                    tcoordGlobalLocal[ ta - 1 ] = (int)gMeshes.back().tcoord.size() - 1;
                }
                
                face.uvInd[ 0 ] = ta - 1;
            }
            else
            {
                stm.clear();
            }

            stm >> slash;
            
            if (slash == '/')
            {
                int na; // Vertex normal indices.
                // Determines the presence of a vertex normal and reads it.
                stm >> na;

                if (!stm.fail())
                {
                    if (na < 0)
                    {
                        na = (int)gMeshes.back().vnormal.size() - na;
                    }
                    
                    hasVNormals = true;

                    if (normGlobalLocal.find(na-1) == normGlobalLocal.end())
                    {
                        gMeshes.back().vnormal.push_back( vnormal[na-1] );
                        normGlobalLocal[ na - 1 ] = (int)gMeshes.back().vnormal.size() - 1;
                    }
                    
                    face.vnInd[ 0 ] = na - 1;
                }
                else
                {
                    stm.clear();
                }
            }
            // Reads the second vertex.
            stm >> vb;
            if (vb < 0)
            {
                vb = (int)gMeshes.back().vertex.size() - vb;
            }

            // Didn't find the index in index conversion map, so add it.
            if (vertGlobalLocal.find(vb-1) == vertGlobalLocal.end())
            {
                gMeshes.back().vertex.push_back( vertex[vb-1] );
                vertGlobalLocal[ vb - 1 ] = (int)gMeshes.back().vertex.size() - 1;
            }

            face.vInd[ 1 ] = vb - 1;

            // Texture coordinate index of this vertex.
            if (hasTextureCoords)
            {
                int tb;
                stm >> slash;
                stm >> tb;
                if (tb < 0)
                {
                    tb = (int)gMeshes.back().tcoord.size() - tb;
                }

                // Didn't find the index in index conversion map, so add it.
                if (tcoordGlobalLocal.find(tb-1) == tcoordGlobalLocal.end())
                {
                    gMeshes.back().tcoord.push_back( tcoord[tb-1] );
                    tcoordGlobalLocal[ tb - 1 ] = (int)gMeshes.back().tcoord.size() - 1;
                }

                face.uvInd[ 1 ] = tb - 1;
            }
            // Eats '/' if face has normals but not texture coords.
            if (!hasTextureCoords && hasVNormals)
            {
                stm >> slash;
            }
            // Vertex normal index of this vertex.
            if (hasVNormals)
            {
                int nb;
                stm >> slash;
                stm >> nb;
                if (nb < 0)
                {
                    nb = (int)gMeshes.back().vnormal.size() - nb;
                }

                // Didn't find the index in index conversion map, so add it.
                if (normGlobalLocal.find(nb-1) == normGlobalLocal.end())
                {
                    gMeshes.back().vnormal.push_back( vnormal[nb-1] );
                    normGlobalLocal[ nb - 1 ] = (int)gMeshes.back().vnormal.size() - 1;
                }

                face.vnInd[ 1 ] = nb - 1;
            }

            // Reads the third vertex.
            stm >> vc;
            if (vc < 0)
            {
                vc = (int)gMeshes.back().vertex.size() - vc;
            }

            // Didn't find the index in index conversion map, so add it.
            if (vertGlobalLocal.find(vc-1) == vertGlobalLocal.end())
            {
                gMeshes.back().vertex.push_back( vertex[vc-1] );
                vertGlobalLocal[ vc - 1 ] = (int)gMeshes.back().vertex.size() - 1;
            }

            face.vInd[ 2 ] = vc - 1;
            
            // Texture coordinate index of this vertex.
            if (hasTextureCoords)
            {
                int tc;
                stm >> slash;
                stm >> tc;
                if (tc < 0)
                {
                    tc = (int)gMeshes.back().tcoord.size() - tc;
                }

                // Didn't find the index in index conversion map, so add it.
                if (tcoordGlobalLocal.find(tc-1) == tcoordGlobalLocal.end())
                {
                    gMeshes.back().tcoord.push_back( tcoord[tc-1] );
                    tcoordGlobalLocal[ tc - 1 ] = (int)gMeshes.back().tcoord.size() - 1;
                }

                face.uvInd[ 2 ] = tc - 1;
            }
            // Eats '/' if face has normals but not texture coords.
            if (!hasTextureCoords && hasVNormals)
            {
                stm >> slash;
            }
            // Vertex normal index of this vertex.
            if (hasVNormals)
            {
                int nc;
                stm >> slash;
                stm >> nc;
                if (nc < 0)
                {
                    nc = (int)gMeshes.back().vnormal.size() - nc;
                }

                // Didn't find the index in index conversion map, so add it.
                if (normGlobalLocal.find(nc-1) == normGlobalLocal.end())
                {
                    gMeshes.back().vnormal.push_back( vnormal[nc-1] );
                    normGlobalLocal[ nc - 1 ] = (int)gMeshes.back().vnormal.size() - 1;
                }

                face.vnInd[ 2 ] = nc - 1;
            }
            gMeshes.back().face.push_back( face );

            // Reads the fourth vertex, if it exists.
            stm >> vd;

            if (!stm.eof())
            {
                if (vd < 0)
                {
                    vd = (int)gMeshes.back().vertex.size() - vd;
                }

                // Didn't find the index in index conversion map, so add it.
                if (vertGlobalLocal.find(vd-1) == vertGlobalLocal.end())
                {
                    gMeshes.back().vertex.push_back( vertex[vd-1] );
                    vertGlobalLocal[ vd - 1 ] = (int)gMeshes.back().vertex.size() - 1;
                }
                Face face2;
                face2.vInd[ 1 ] = vd - 1;

                face2.vInd[0] = face.vInd[2];
                face2.uvInd[0] = face.uvInd[2];
                face2.vnInd[0] = face.vnInd[2];

                face2.vInd[2] = face.vInd[0];
                face2.uvInd[2] = face.uvInd[0];
                face2.vnInd[2] = face.vnInd[0];

                // Texture coordinate index of this vertex.
                if (hasTextureCoords)
                {
                    int td;
                    stm >> slash;
                    stm >> td;
                    if (td < 0)
                    {
                        td = (int)gMeshes.back().tcoord.size() - td;
                    }

                    // Didn't find the index in index conversion map, so add it.
                    if (tcoordGlobalLocal.find(td-1) == tcoordGlobalLocal.end())
                    {
                        gMeshes.back().tcoord.push_back( tcoord[td-1] );
                        tcoordGlobalLocal[ td - 1 ] = (int)gMeshes.back().tcoord.size() - 1;
                    }

                    face2.uvInd[ 1 ] = td - 1;
                }
                // Eats '/' if the mesh has normals but not texture coords.
                if (!hasTextureCoords && hasVNormals)
                {
                    stm >> slash;
                }
                // Vertex normal index of this vertex.
                if (hasVNormals)
                {
                    int nd;
                    stm >> slash;
                    stm >> nd;
                    if (nd < 0)
                    {
                        nd = (int)gMeshes.back().vnormal.size() - nd;
                    }

                    // Didn't find the index in index conversion map, so add it.
                    if (normGlobalLocal.find(nd-1) == normGlobalLocal.end())
                    {
                        gMeshes.back().vnormal.push_back( vnormal[nd-1] );
                        normGlobalLocal[ nd - 1 ] = (int)gMeshes.back().vnormal.size() - 1;
                    }

                    face2.vnInd[ 1 ] = nd - 1;
                }
                gMeshes.back().face.push_back( face2 );
            }
        }
    }
    
    ConvertIndices();
}

int main( int paramCount, char** params )
{
    if (paramCount != 3)
    {
        std::cerr << "Usage: ./convert_obj <vertexformat> file.obj" << std::endl;
        std::cerr << "  where <vertexformat> is 0 for PTNTC and 1 for PTN." << std::endl;
        return 1;
    }

    std::cerr << "Converting... ";

    LoadObj( params[ 2 ] );

    // Creates a new file name by replacing 'obj' with 'ae3d'.
    std::string outFile = std::string( params [ 2 ] );
    outFile = outFile.substr( 0, outFile.length() - 3 );
    outFile.append( "ae3d" );
    
    VertexFormat vertexFormat { VertexFormat::PTNTC };
    
    if (std::string( params[ 1 ] ) == "1")
    {
        vertexFormat = VertexFormat::PTN;
    }
    
    WriteAe3d( outFile, vertexFormat );
    return 0;
}
