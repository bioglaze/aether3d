#ifdef SIMD_SSE3
#include "Matrix.hpp"
#include <pmmintrin.h>
#include "Vec3.hpp"

using namespace ae3d;

void Matrix44::Multiply( const Matrix44& ma, const Matrix44& mb, Matrix44& out )
{
    Matrix44 result;
    Matrix44 matA = ma;
    Matrix44 matB = mb;

    const float* a = matA.m;
    const float* b = matB.m;
    __m128 a_line, b_line, r_line;

    for (int i = 0; i < 16; i += 4)
    {
        // unroll the first step of the loop to avoid having to initialize r_line to zero
        a_line = _mm_load_ps( b );
        b_line = _mm_set1_ps( a[ i ] );
        r_line = _mm_mul_ps( a_line, b_line );

        for (int j = 1; j < 4; j++)
        {
            a_line = _mm_load_ps( &b[ j * 4 ] );
            b_line = _mm_set1_ps(  a[ i + j ] );
            r_line = _mm_add_ps(_mm_mul_ps( a_line, b_line ), r_line);
        }

        _mm_store_ps( &result.m[ i ], r_line );
    }

    out = result;
}

void Matrix44::TransformPoint( const Vec4& vec, const Matrix44& mat, Vec4* out )
{
    Matrix44 transpose;
    mat.Transpose( transpose );
    alignas( 16 ) Vec4 v4 = vec;

    __m128 vec4 = _mm_load_ps( &v4.x );
    __m128 row1 = _mm_load_ps( &transpose.m[  0 ] );
    __m128 row2 = _mm_load_ps( &transpose.m[  4 ] );
    __m128 row3 = _mm_load_ps( &transpose.m[  8 ] );
    __m128 row4 = _mm_load_ps( &transpose.m[ 12 ] );

    __m128 r1 = _mm_mul_ps( row1, vec4 );
    __m128 r2 = _mm_mul_ps( row2, vec4 );
    __m128 r3 = _mm_mul_ps( row3, vec4 );
    __m128 r4 = _mm_mul_ps( row4, vec4 );

    __m128 sum_01 = _mm_hadd_ps( r1, r2 );
    __m128 sum_23 = _mm_hadd_ps( r3, r4 );
    __m128 result = _mm_hadd_ps( sum_01, sum_23 );
    _mm_store_ps( &out->x, result );
}
#endif
