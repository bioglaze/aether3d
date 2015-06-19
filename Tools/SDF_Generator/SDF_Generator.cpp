/**
 Based on http://metalbyexample.com/rendering-text-in-metal-with-signed-distance-fields/
*/
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdint.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.c"

std::vector< float > distanceMap;
std::vector< float > scaledDistanceMap;
int scaledWidth = 0;
int scaledHeight = 0;

void ScaleDistanceMap( int width, int height, int scale )
{
    if (width % scale != 0 || height % scale != 0)
    {
        std::cerr << "Uneven scale " << scale << " for image of size " << width << "x" << height << std::endl;
    }

    scaledWidth = width / scale;
    scaledHeight = height / scale;

    scaledDistanceMap.resize( scaledWidth * scaledHeight );
    
    for (int y = 0; y < height; y += scale)
    {
        for (int x = 0; x < width; x += scale)
        {
            float accum = 0;
            
            for (int ky = 0; ky < scale; ++ky)
            {
                for (int kx = 0; kx < scale; ++kx)
                {
                    accum += distanceMap[ (y + ky) * width + (x + kx) ];
                }
            }
            
            accum = accum / (scale * scale);
            
            scaledDistanceMap[(y / scale) * scaledWidth + (x / scale)] = accum;
        }
    }
}

// imageData must have only one color channel.
void CreateDistanceMap( unsigned char* imageData, int width, int height )
{
    typedef struct { unsigned short x, y; } intpoint_t;

    // Distance to nearest boundary point map.
    distanceMap.resize( width * height );

    for (std::size_t i = 0; i < distanceMap.size(); ++i)
    {
        distanceMap[i] = 0;
    }
    
    // Nearest boundary point map.
    intpoint_t *boundaryPointMap = new intpoint_t[ width * height ];

    const float maxDist = hypot( width, height );

    // Initialization phase: set all distances to "infinity"; zero out nearest boundary point map
    for (long y = 0; y < height; ++y)
    {
        for (long x = 0; x < width; ++x)
        {
            distanceMap[ y * width + x ] = maxDist;
            boundaryPointMap[ y * width + x ] = { 0, 0 };
        }
    }

#define image(_x, _y) (imageData[(_y) * width + (_x)] > 0x7f)

    // Immediate interior/exterior phase: mark all points along the boundary as such
    for (unsigned short y = 1; y < height - 1; ++y)
    {
        for (unsigned short x = 1; x < width - 1; ++x)
        {
            bool inside = image(x, y);
            if (image(x - 1, y) != inside ||
                image(x + 1, y) != inside ||
                image(x, y - 1) != inside ||
                image(x, y + 1) != inside)
            {
                distanceMap[ y * width + x ] = 0;
                boundaryPointMap[ y * width + x ] = { x, y };
            }
        }
    }

#define distance(_x, _y) distanceMap[(_y) * width + (_x)]
#define nearestpt(_x, _y) boundaryPointMap[(_y) * width + (_x)]
#define image(_x, _y) (imageData[(_y) * width + (_x)] > 0x7f)

    const float distUnit = 1;
    const float distDiag = sqrt(2);

    // Forward dead-reckoning pass
    for (long y = 1; y < height - 2; ++y)
    {
        for (long x = 1; x < width - 2; ++x)
        {
            if (distanceMap[(y - 1) * width + (x - 1)] + distDiag < distanceMap[ y * width + x ])
            {
                nearestpt(x, y) = nearestpt(x - 1, y - 1);
                distanceMap[ y * width + x ] = hypot(x - nearestpt(x, y).x, y - nearestpt(x, y).y);
            }
            if (distanceMap[ (y - 1) * width + x ] + distUnit < distanceMap[ y * width + x ])
            {
                nearestpt(x, y) = nearestpt(x, y - 1);
                distanceMap[ y * width + x ] = hypot(x - nearestpt(x, y).x, y - nearestpt(x, y).y);
            }
            if (distanceMap[ (y-1) * width + (x+1) ] + distDiag < distanceMap[ y * width + x ])
            {
                nearestpt(x, y) = nearestpt(x + 1, y - 1);
                distanceMap[ y * width + x ] = hypot(x - nearestpt(x, y).x, y - nearestpt(x, y).y);
            }
            if (distanceMap[ y * width + x - 1 ] + distUnit < distanceMap[ y * width + x ])
            {
                nearestpt(x, y) = nearestpt(x - 1, y);
                distanceMap[ y * width + x ] = hypot(x - nearestpt(x, y).x, y - nearestpt(x, y).y);
            }
        }
    }

    // Backward dead-reckoning pass
    for (long y = height - 2; y >= 1; --y)
    {
        for (long x = width - 2; x >= 1; --x)
        {
            if (distance(x + 1, y) + distUnit < distance(x, y))
            {
                nearestpt(x, y) = nearestpt(x + 1, y);
                distance(x, y) = hypot(x - nearestpt(x, y).x, y - nearestpt(x, y).y);
            }
            if (distance(x - 1, y + 1) + distDiag < distance(x, y))
            {
                nearestpt(x, y) = nearestpt(x - 1, y + 1);
                distance(x, y) = hypot(x - nearestpt(x, y).x, y - nearestpt(x, y).y);
            }
            if (distance(x, y + 1) + distUnit < distance(x, y))
            {
                nearestpt(x, y) = nearestpt(x, y + 1);
                distance(x, y) = hypot(x - nearestpt(x, y).x, y - nearestpt(x, y).y);
            }
            if (distance(x + 1, y + 1) + distDiag < distance(x, y))
            {
                nearestpt(x, y) = nearestpt(x + 1, y + 1);
                distance(x, y) = hypot(x - nearestpt(x, y).x, y - nearestpt(x, y).y);
            }
        }
    }
    
    // Interior distance negation pass; distances outside the figure are considered negative
    for (long y = 0; y < height; ++y)
    {
        for (long x = 0; x < width; ++x)
        {
            if (!image(x, y))
            {
                distance(x, y) = -distance(x, y);
            }
        }
    }
    
    free(boundaryPointMap);
}

void WriteScaledMapIntoTGAFile( const char* path, int width, int height )
{
    std::ofstream ofs( path );
    
    uint8_t idLen = 0;
    ofs.write( (char*) &idLen, 1 );
    
    uint8_t indexed = 0;
    ofs.write( (char*) &indexed, 1 );
    
    uint8_t imageType = 2; // Uncompressed RGB
    ofs.write( (char*) &imageType, 1 );
    
    uint16_t paletteStart = 0;
    ofs.write( (char*) &paletteStart, 2 );
    
    uint16_t paletteLen = 0;
    ofs.write( (char*) &paletteLen, 2 );

    uint8_t paletteBits = 0;
    ofs.write( (char*) &paletteBits, 1 );

    uint16_t xOrigin = 0;
    ofs.write( (char*) &xOrigin, 2 );

    uint16_t yOrigin = 0;
    ofs.write( (char*) &yOrigin, 2 );

    ofs.write( (char*) &width, 2 );
    ofs.write( (char*) &height, 2 );

    uint8_t bpp = 24;
    ofs.write( (char*) &bpp, 1 );

    uint8_t inverted = 0;
    ofs.write( (char*) &inverted, 1 );
    
    for (int i = 0; i < width * height; ++i)
    {
        const float normalizationFactor = 16;
        const float dist = scaledDistanceMap[ i ];
        const float clampDist = fmax(-normalizationFactor, fmin(dist, normalizationFactor));
        const float scaledDist = clampDist / normalizationFactor;
        const uint8_t value = ((scaledDist + 1) / 2) * UINT8_MAX;

        ofs.write( (char*) &value, 1); // blue
        ofs.write( (char*) &value, 1); // green
        ofs.write( (char*) &value, 1); // red
    }
}

int main( int argCount, char* args[] )
{
    if (argCount != 3)
    {
        std::cout << "Usage: SDF_Generator font.png font_sdf.tga" << std::endl;
        return 1;
    }

    int width, height, components;
    unsigned char* imageData = stbi_load( args[1], &width, &height, &components, 1 );

    if (imageData == nullptr)
    {
        std::cerr << "Failed to load " << args[ 1 ] << ". Reason: " << stbi_failure_reason() << std::endl;
        return 1;
    }

    CreateDistanceMap( imageData, width, height );
    ScaleDistanceMap( width, height, 4 );
    WriteScaledMapIntoTGAFile( "sdf.tga", scaledWidth, scaledHeight );

    stbi_image_free( imageData );
    return 0;
}
