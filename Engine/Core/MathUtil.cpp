#include <vector>
#include "Vec3.hpp"

using namespace ae3d;

namespace MathUtil
{
    void GetMinMax( const std::vector< Vec3 >& aPoints, Vec3& outMin, Vec3& outMax )
    {
        if (!aPoints.empty())
        {
            outMin = aPoints[ 0 ];
            outMax = aPoints[ 0 ];
        }

        for (std::size_t i = 1, s = aPoints.size(); i < s; ++i)
        {
            const Vec3& point = aPoints[ i ];

            if (point.x < outMin.x)
            {
                outMin.x = point.x;
            }

            if (point.y < outMin.y)
            {
                outMin.y = point.y;
            }

            if (point.z < outMin.z)
            {
                outMin.z = point.z;
            }

            if (point.x > outMax.x)
            {
                outMax.x = point.x;
            }

            if (point.y > outMax.y)
            {
                outMax.y = point.y;
            }

            if (point.z > outMax.z)
            {
                outMax.z = point.z;
            }
        }
    }

    void GetCorners( const Vec3& min, const Vec3& max, std::vector< Vec3 >& outCorners )
    {
        outCorners =
        {
            Vec3( min.x, min.y, min.z ),
            Vec3( max.x, min.y, min.z ),
            Vec3( min.x, max.y, min.z ),
            Vec3( min.x, min.y, max.z ),
            Vec3( max.x, max.y, min.z ),
            Vec3( min.x, max.y, max.z ),
            Vec3( max.x, max.y, max.z ),
            Vec3( max.x, min.y, max.z )
        };
    }

    float Floor( float f )
    {
        return std::floor( f );
    }

    bool IsNaN( float f )
    {
        return f != f;
    }

    bool IsFinite( float f )
    {
        return std::isfinite( f );
    }

    bool IsPowerOfTwo( unsigned i )
    {
        return ((i & (i - 1)) == 0);
    }

    unsigned GetHash( const char* s, unsigned length )
    {
        const unsigned A = 54059;
        const unsigned B = 76963;

        unsigned h = 31;
        unsigned i = 0;

        while (i < length)
        {
            h = (h * A) ^ (s[ 0 ] * B);
            ++s;
            ++i;
        }

        return h;
    }

    int Min( int x, int y )
    {
        return x < y ? x : y;
    }

    int Max( int x, int y )
    {
        return x > y ? x : y;
    }

    int GetMipmapCount( int width, int height )
    {
        return 1 + static_cast< int >(std::floor( std::log2( Min( width, height ) ) ));
    }
}
