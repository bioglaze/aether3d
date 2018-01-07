#if RENDERER_METAL && !(__i386__)
#include "Matrix.hpp"
#include <cstring>
#include <cmath>
#include <arm_neon.h>
#include "Vec3.hpp"

#ifndef M_PI
#    define M_PI 3.14159265358979f
#endif

using namespace ae3d;

void Matrix44::Multiply( const Matrix44& ma, const Matrix44& mb, Matrix44& out )
{
    Matrix44 result;
    Matrix44 matA = ma;
    Matrix44 matB = mb;
    
    const float* a = matA.m;
    const float* b = matB.m;
    float32x4_t a_line, b_line, r_line;
    
    for (int i = 0; i < 16; i += 4)
    {
        // unroll the first step of the loop to avoid having to initialize r_line to zero
        a_line = vld1q_f32( b );
        b_line = vld1q_dup_f32( &a[ i ] );
        r_line = vmulq_f32( a_line, b_line );
        
        for (int j = 1; j < 4; j++)
        {
            a_line = vld1q_f32( &b[ j * 4 ] );
            b_line = vdupq_n_f32(  a[ i + j ] );
            r_line = vaddq_f32(vmulq_f32( a_line, b_line ), r_line);
        }
        
        vst1q_f32( &result.m[ i ], r_line );
    }
    
    out = result;
}

void ae3d::Matrix44::TransformPoint( const Vec4& vec, const Matrix44& mat, Vec4* out )
{
    Vec4 tmp;
    tmp.x = mat.m[0] * vec.x + mat.m[ 4 ] * vec.y + mat.m[ 8] * vec.z + mat.m[12] * vec.w;
    tmp.y = mat.m[1] * vec.x + mat.m[ 5 ] * vec.y + mat.m[ 9] * vec.z + mat.m[13] * vec.w;
    tmp.z = mat.m[2] * vec.x + mat.m[ 6 ] * vec.y + mat.m[10] * vec.z + mat.m[14] * vec.w;
    tmp.w = mat.m[3] * vec.x + mat.m[ 7 ] * vec.y + mat.m[11] * vec.z + mat.m[15] * vec.w;
    
    *out = tmp;
    
#if AE3D_CHECK_FOR_NAN
    ae3d::CheckNaN( mat );
#endif
}

#endif
