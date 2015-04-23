#ifndef SIMD_SSE3
#include "Matrix.hpp"
#include <cstring>
#include <cmath>
#include "Vec3.hpp"

#ifndef M_PI
#    define M_PI 3.14159265358979f
#endif

namespace
{
bool IsNaN( float f )
{
    return f != f;
}
}

namespace ae3d
{
const float biasDataColMajor[] =
  { 0.5f, 0, 0, 0,
    0, 0.5f, 0, 0,
    0, 0, 0.5f, 0,
    0.5f, 0.5f, 0.5f, 1 };

    void CheckNaN( const Matrix44& matrix )
    {
        for (int i = 0; i < 16; ++i)
        {
            if (IsNaN( matrix.m[ i ] ))
            {
                //std::cerr << "Matrix contains NaN" << std::endl;
            }
        }
    }
}

using namespace ae3d;

const Matrix44 Matrix44::identity;
const Matrix44 Matrix44::bias( ae3d::biasDataColMajor );

void Matrix44::InverseTranspose( const float m[ 16 ], float* out )
{
    float tmp[ 12 ];
    
    // Calculates pairs for first 8 elements (cofactors)
    tmp[  0 ] = m[ 10 ] * m[ 15 ];
    tmp[  1 ] = m[ 11 ] * m[ 14 ];
    tmp[  2 ] = m[  9 ] * m[ 15 ];
    tmp[  3 ] = m[ 11 ] * m[ 13 ];
    tmp[  4 ] = m[  9 ] * m[ 14 ];
    tmp[  5 ] = m[ 10 ] * m[ 13 ];
    tmp[  6 ] = m[  8 ] * m[ 15 ];
    tmp[  7 ] = m[ 11 ] * m[ 12 ];
    tmp[  8 ] = m[  8 ] * m[ 14 ];
    tmp[  9 ] = m[ 10 ] * m[ 12 ];
    tmp[ 10 ] = m[  8 ] * m[ 13 ];
    tmp[ 11 ] = m[  9 ] * m[ 12 ];
    
    // Calculates first 8 elements (cofactors)
    out[ 0 ] = tmp[0]*m[5] + tmp[3]*m[6] + tmp[4]*m[7] - tmp[1]*m[5] - tmp[2]*m[6] - tmp[5]*m[7];
    out[ 1 ] = tmp[1]*m[4] + tmp[6]*m[6] + tmp[9]*m[7] - tmp[0]*m[4] - tmp[7]*m[6] - tmp[8]*m[7];
    out[ 2 ] = tmp[2]*m[4] + tmp[7]*m[5] + tmp[10]*m[7] - tmp[3]*m[4] - tmp[6]*m[5] - tmp[11]*m[7];
    out[ 3 ] = tmp[5]*m[4] + tmp[8]*m[5] + tmp[11]*m[6] - tmp[4]*m[4] - tmp[9]*m[5] - tmp[10]*m[6];
    out[ 4 ] = tmp[1]*m[1] + tmp[2]*m[2] + tmp[5]*m[3] - tmp[0]*m[1] - tmp[3]*m[2] - tmp[4]*m[3];
    out[ 5 ] = tmp[0]*m[0] + tmp[7]*m[2] + tmp[8]*m[3] - tmp[1]*m[0] - tmp[6]*m[2] - tmp[9]*m[3];
    out[ 6 ] = tmp[3]*m[0] + tmp[6]*m[1] + tmp[11]*m[3] - tmp[2]*m[0] - tmp[7]*m[1] - tmp[10]*m[3];
    out[ 7 ] = tmp[4]*m[0] + tmp[9]*m[1] + tmp[10]*m[2] - tmp[5] * m[0] - tmp[8] * m[1] - tmp[11] * m[2];
    
    // Calculates pairs for second 8 elements (cofactors)
    tmp[  0 ] = m[ 2 ] * m[ 7 ];
    tmp[  1 ] = m[ 3 ] * m[ 6 ];
    tmp[  2 ] = m[ 1 ] * m[ 7 ];
    tmp[  3 ] = m[ 3 ] * m[ 5 ];
    tmp[  4 ] = m[ 1 ] * m[ 6 ];
    tmp[  5 ] = m[ 2 ] * m[ 5 ];
    tmp[  6 ] = m[ 0 ] * m[ 7 ];
    tmp[  7 ] = m[ 3 ] * m[ 4 ];
    tmp[  8 ] = m[ 0 ] * m[ 6 ];
    tmp[  9 ] = m[ 2 ] * m[ 4 ];
    tmp[ 10 ] = m[ 0 ] * m[ 5 ];
    tmp[ 11 ] = m[ 1 ] * m[ 4 ];
    
    // Calculates second 8 elements (cofactors)
    out[  8 ] = tmp[0] * m[13] + tmp[3] * m[14] + tmp[4] * m[15]
    -  tmp[1]*m[13] - tmp[2]*m[14] - tmp[5]*m[15];
    
    out[  9 ] = tmp[1] * m[12] + tmp[6] * m[14] + tmp[9] * m[15]
    -  tmp[0]*m[12] - tmp[7]*m[14] - tmp[8]*m[15];
    
    out[ 10 ] = tmp[2] * m[12] + tmp[7] * m[13] + tmp[10] * m[15]
    -  tmp[3]*m[12] - tmp[6]*m[13] - tmp[11]*m[15];
    
    out[ 11 ] = tmp[5] * m[12] + tmp[8] * m[13] + tmp[11] * m[14]
    -  tmp[4]*m[12] - tmp[9]*m[13] - tmp[10]*m[14];
    
    out[ 12 ] = tmp[2] * m[10] + tmp[5] * m[11] + tmp[1] * m[9]
    -  tmp[4]*m[11] - tmp[0]*m[9] - tmp[3]*m[10];
    
    out[ 13 ] = tmp[8] * m[11] + tmp[0] * m[8] + tmp[7] * m[10]
    -  tmp[6]*m[10] - tmp[9]*m[11] - tmp[1]*m[8];
    
    out[ 14 ] = tmp[6] * m[9] + tmp[11] * m[11] + tmp[3] * m[8]
    -  tmp[10]*m[11] - tmp[2]*m[8] - tmp[7]*m[9];
    
    out[ 15 ] = tmp[10] * m[10] + tmp[4] * m[8] + tmp[9] * m[9]
    -  tmp[8]*m[9] - tmp[11]*m[10] - tmp[5]*m[8];
    
    // Calculates the determinant.
    const float det = m[ 0 ] * out[ 0 ] + m[ 1 ] * out[ 1 ] + m[ 2 ] * out[ 2 ] + m[ 3 ] * out[ 3 ];
    const float acceptableDelta = 0.0001f;
    
    if (std::fabs( det ) < acceptableDelta)
    {
        std::memcpy( out, identity.m, sizeof( Matrix44 ) );
        return;
    }
    for (int i = 0; i < 16; ++i)
    {
        out[ i ] /= det;
    }
    
#if DEBUG
    //ae3d::CheckNaN( out );
#endif
}

void Matrix44::Invert( const Matrix44& matrix, Matrix44& out )
{
    float invTrans[ 16 ];
    InverseTranspose( matrix.m, invTrans );
    Matrix44 iTrans;
    iTrans.InitFrom( invTrans );
    iTrans.Transpose( out );
    
#if DEBUG
    ae3d::CheckNaN( out );
#endif
}

void Matrix44::Multiply( const Matrix44& a, const Matrix44& b, Matrix44& out )
{
    float tmp[ 16 ];

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            tmp[ i * 4 + j ] = a.m[ i * 4 + 0 ] * b.m[ 0 * 4 + j ] +
                               a.m[ i * 4 + 1 ] * b.m[ 1 * 4 + j ] +
                               a.m[ i * 4 + 2 ] * b.m[ 2 * 4 + j ] +
                               a.m[ i * 4 + 3 ] * b.m[ 3 * 4 + j ];
        }
    }

    out.InitFrom( tmp );
#if DEBUG
    ae3d::CheckNaN( out );
#endif
}

void ae3d::Matrix44::TransformPoint( const Vec4& vec, const Matrix44& mat, Vec4* out )
{
    Vec4 tmp;
    tmp.x = mat.m[0] * vec.x + mat.m[ 4 ] * vec.y + mat.m[ 8] * vec.z + mat.m[12] * vec.w;
    tmp.y = mat.m[1] * vec.x + mat.m[ 5 ] * vec.y + mat.m[ 9] * vec.z + mat.m[13] * vec.w;
    tmp.z = mat.m[2] * vec.x + mat.m[ 6 ] * vec.y + mat.m[10] * vec.z + mat.m[14] * vec.w;
    tmp.w = mat.m[3] * vec.x + mat.m[ 7 ] * vec.y + mat.m[11] * vec.z + mat.m[15] * vec.w;

    *out = tmp;

#if DEBUG
    ae3d::CheckNaN( mat );
#endif
}

void Matrix44::TransformPoint( const Vec3& vec, const Matrix44& mat, Vec3* out )
{
    Vec3 tmp;
    tmp.x = mat.m[0] * vec.x + mat.m[ 4 ] * vec.y + mat.m[ 8] * vec.z + mat.m[12];
    tmp.y = mat.m[1] * vec.x + mat.m[ 5 ] * vec.y + mat.m[ 9] * vec.z + mat.m[13];
    tmp.z = mat.m[2] * vec.x + mat.m[ 6 ] * vec.y + mat.m[10] * vec.z + mat.m[14];

    *out = tmp;
    
#if DEBUG
    ae3d::CheckNaN( mat );
#endif
}

void Matrix44::TransformDirection( const Vec3& dir, const Matrix44& mat, Vec3* out )
{
    Vec3 tmp;

    tmp.x = mat.m[0] * dir.x + mat.m[ 4 ] * dir.y + mat.m[ 8] * dir.z;
    tmp.y = mat.m[1] * dir.x + mat.m[ 5 ] * dir.y + mat.m[ 9] * dir.z;
    tmp.z = mat.m[2] * dir.x + mat.m[ 6 ] * dir.y + mat.m[10] * dir.z;

    *out = tmp;
    
#if DEBUG
    ae3d::CheckNaN( mat );
#endif
}

Matrix44::Matrix44( const Matrix44& other )
{
    InitFrom( other.m );

#if DEBUG
    ae3d::CheckNaN( other );
#endif
}

Matrix44::Matrix44( const float* data )
{
    InitFrom( data );

#if DEBUG
    ae3d::CheckNaN( *this );
#endif
}

void Matrix44::InitFrom( const float* data )
{
    std::memcpy( m, data, sizeof( m ) );
}

void Matrix44::MakeIdentity()
{
    std::memset( m, 0, sizeof( m ) );

    m[  0 ] = 1;
    m[  5 ] = 1;
    m[ 10 ] = 1;
    m[ 15 ] = 1;
}

void Matrix44::MakeLookAt( const Vec3& eye, const Vec3& center, const Vec3& up )
{
    Vec3 forward = (center - eye).Normalized();

    Vec3 right = Vec3::Cross( forward, up ).Normalized();

    const Vec3 newUp = Vec3::Cross( right, forward );

    m[ 0 ] = right.x;
    m[ 4 ] = right.y;
    m[ 8 ] = right.z;
    m[ 12 ] = 0;

    m[ 1 ] = newUp.x;
    m[ 5 ] = newUp.y;
    m[ 9 ] = newUp.z;
    m[13 ] = 0;

    m[ 2 ] = -forward.x;
    m[ 6 ] = -forward.y;
    m[10 ] = -forward.z;
    m[14 ] = 0;

    m[ 3 ] = m[ 7 ] = m[ 11 ] = 0;
    m[15 ] = 1;

    Matrix44 translate;
    translate.MakeIdentity();
    translate.m[ 12 ] = -eye.x;
    translate.m[ 13 ] = -eye.y;
    translate.m[ 14 ] = -eye.z;

    Multiply( translate, *this, *this );
    
#if DEBUG
    ae3d::CheckNaN( *this );
#endif
}

void Matrix44::MakeProjection( float fovDegrees, float aspect, float nearDepth, float farDepth )
{
    const float top = std::tan( fovDegrees * (static_cast< float >( M_PI ) / 360.0f) ) * nearDepth;
    const float bottom = -top;
    const float left = aspect * bottom;
    const float right = aspect * top;

    const float x = (2 * nearDepth) / (right - left);
    const float y = (2 * nearDepth) / (top - bottom);
    const float a = (right + left)  / (right - left);
    const float b = (top + bottom)  / (top - bottom);

    const float c = -(farDepth + nearDepth) / (farDepth - nearDepth);
    const float d = -(2 * farDepth * nearDepth) / (farDepth - nearDepth);

    const float proj[] =
    {
        x, 0, 0,  0,
        0, y, 0,  0,
        a, b, c, -1,
        0, 0, d,  0
    };

    InitFrom( proj );  
#if DEBUG
    ae3d::CheckNaN( *this );
#endif
}

void Matrix44::MakeProjection( float left, float right, float bottom, float top, float nearDepth, float farDepth )
{
    const float tx = -((right + left) / (right - left));
    const float ty = -((top + bottom) / (top - bottom));
    const float tz = -((farDepth + nearDepth) / (farDepth - nearDepth));

    const float ortho[ 16 ] =
    {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
        0.0f, 0.0f, -2.0f / (farDepth - nearDepth), 0.0f,
        tx, ty, tz, 1.0f
    };

    InitFrom( ortho );
    
#if DEBUG
    ae3d::CheckNaN( *this );
#endif
}

void Matrix44::MakeRotationXYZ( float xDeg, float yDeg, float zDeg )
{
    const float deg2rad = static_cast< float >( M_PI ) / 180.0f;
    const float sx = std::sin( xDeg * deg2rad );
    const float sy = std::sin( yDeg * deg2rad );
    const float sz = std::sin( zDeg * deg2rad );
    const float cx = std::cos( xDeg * deg2rad );
    const float cy = std::cos( yDeg * deg2rad );
    const float cz = std::cos( zDeg * deg2rad );

    m[ 0 ] = cy * cz;
    m[ 1 ] = cz * sx * sy - cx * sz;
    m[ 2 ] = cx * cz * sy + sx * sz;
    m[ 3 ] = 0;
    m[ 4 ] = cy * sz;
    m[ 5 ] = cx * cz + sx * sy * sz;
    m[ 6 ] = -cz * sx + cx * sy * sz;
    m[ 7 ] = 0;
    m[ 8 ] = -sy;
    m[ 9 ] = cy * sx;
    m[10 ] = cx * cy;
    m[11 ] = 0;
    m[12 ] = 0;
    m[13 ] = 0;
    m[14 ] = 0;
    m[15 ] = 1;
    
#if DEBUG
    ae3d::CheckNaN( *this );
#endif
}

void Matrix44::Scale( float x, float y, float z )
{
    /*if (Vec3( x, y, z ).IsAlmost( Vec3( 1, 1, 1 ) ))
    {
        return;
    }*/

    Matrix44 scale;

    scale.m[  0 ] = x;
    scale.m[  5 ] = y;
    scale.m[ 10 ] = z;

    Multiply( *this, scale, *this );
}

void Matrix44::Transpose( Matrix44& out ) const
{
    float tmp[ 16 ];

    tmp[  0 ] = m[  0 ];
    tmp[  1 ] = m[  4 ];
    tmp[  2 ] = m[  8 ];
    tmp[  3 ] = m[ 12 ];
    tmp[  4 ] = m[  1 ];
    tmp[  5 ] = m[  5 ];
    tmp[  6 ] = m[  9 ];
    tmp[  7 ] = m[ 13 ];
    tmp[  8 ] = m[  2 ];
    tmp[  9 ] = m[  6 ];
    tmp[ 10 ] = m[ 10 ];
    tmp[ 11 ] = m[ 14 ];
    tmp[ 12 ] = m[  3 ];
    tmp[ 13 ] = m[  7 ];
    tmp[ 14 ] = m[ 11 ];
    tmp[ 15 ] = m[ 15 ];

    out.InitFrom( tmp );
    
#if DEBUG
    ae3d::CheckNaN( out );
#endif
}

void Matrix44::Translate( const Vec3& v )
{
    Matrix44 translateMatrix;

    translateMatrix.m[ 12 ] = v.x;
    translateMatrix.m[ 13 ] = v.y;
    translateMatrix.m[ 14 ] = v.z;

    Multiply( *this, translateMatrix, *this );
    
#if DEBUG
    ae3d::CheckNaN( *this );
#endif
}
#endif
