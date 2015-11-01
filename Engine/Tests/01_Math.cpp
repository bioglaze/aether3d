#include <iostream>
#include "Vec3.hpp"
#include "Matrix.hpp"
#include "Quaternion.hpp"

using namespace ae3d;

bool IsAlmost( float f1, float f2 )
{
    const float tolerance = 0.0001f;
    return std::abs( f1 - f2 ) < tolerance;
}

void TestVec4()
{
    if (!Vec4( 1, 2, 3, 4 ).IsAlmost( Vec4( 1, 2, 3, 4 ) ))
    {
        std::cerr << "Vec4 IsAlmost or constructor failed!" << std::endl;
    }

    Vec4 vec1;

    if (!vec1.IsAlmost( Vec4( 0, 0, 0, 0 ) ))
    {
        std::cerr << "Vec4 constructor failed!" << std::endl;
    }

    Vec4 vec2( { 1, 2, 3 } );

    if (!vec2.IsAlmost( Vec4( 1, 2, 3, 1 ) ))
    {
        std::cerr << "Vec4 constructor failed!" << std::endl;
    }

}

void TestVec3()
{
    Vec3 vec1;

    if (!vec1.IsAlmost( Vec3( 0, 0, 0 ) ))
    {
        std::cerr << "Vec3 constructor failed!" << std::endl;
    }

    vec1 = Vec3( 2, 3, 2 );
    Vec3 vec2( 3, 2, 3 );

    if (!(vec1 + vec2).IsAlmost( Vec3( 5, 5, 5 ) ))
    {
        std::cerr << "Vec3 addition failed!" << std::endl;
    }

    if (!(vec1 - vec2).IsAlmost( Vec3( -1, 1, -1 ) ))
    {
        std::cerr << "Vec3 subtract failed!" << std::endl;
    }

    if (!(vec1 * vec2).IsAlmost( Vec3( 6, 6, 6 ) ))
    {
        std::cerr << "Vec3 multiply failed!" << std::endl;
    }

    vec2 = vec2.Normalized();

    if (std::abs( vec2.Length() - 1 ) > 0.0001f)
    {
        std::cerr << "Vec3::Normalize failed!" << std::endl;
    }
    
    if (!(Vec3( 1, 2, 3 ) / 2.0f).IsAlmost( Vec3( 0.5f, 1.0f, 1.5f ) ))
    {
        std::cerr << "Vec3 division failed!" << std::endl;
    }

    if (!(Vec3( 1, 1, 1 ) / Vec3( 2, 2, 2 )).IsAlmost( Vec3( 0.5f, 0.5f, 0.5f ) ))
    {
        std::cerr << "Vec3 division failed!" << std::endl;
    }

    if (!(Vec3( 1, 2, 3 ) * 2.0f).IsAlmost( Vec3( 2.0f, 4.0f, 6.0f ) ))
    {
        std::cerr << "Vec3 multiply by value failed!" << std::endl;
    }

    Vec3 a( 1, 2, 3 );
    Vec3 b = a;

    if (!a.IsAlmost( b ))
    {
        std::cerr << "Vec3 assignment failed!" << std::endl;
    }

    if (!Vec3{ 3, 3, 3 }.IsAlmost( { 3, 3, 3 } ))
    {
        std::cerr << "Vec3 IsAlmost failed!" << std::endl;
    }
    
    b = Vec3( 1, 1, 1 );
    Vec3 r = a + b - b;

    if (!r.IsAlmost( a ))
    {
        std::cerr << "Vec3 identity failed!" << std::endl;
    }

    b = Vec3( 2, 2, 2 );
    r = a / b;

    if (!(r * b).IsAlmost( a ))
    {
        std::cerr << "Vec3 identity failed!" << std::endl;
    }

    if (!Vec3::Min2( { 1, 2, 3 }, { 3, 1, 2 } ).IsAlmost( { 1, 1, 2 } ))
    {
        std::cerr << "Vec3 Min2 failed!" << std::endl;
    }
    if (!Vec3::Max2( { 1, 2, 3 }, { 3, 1, 2 } ).IsAlmost( { 3, 2, 3 } ))
    {
        std::cerr << "Vec3 Max2 failed!" << std::endl;
    }

    if (std::abs( Vec3::Dot( { 2, 2, 2 }, { 2, 2, 2 } ) - 12 ) > 0.0001f)
    {
        std::cerr << "Vec3 Dot failed! " << std::endl;
    }

    if (!Vec3::Cross( { 1, 0, 0 }, { 0, 1, 0 } ).IsAlmost( { 0, 0, 1 } ))
    {
        std::cerr << "Vec3 Cross failed!" << std::endl;
    }

    if (std::abs( Vec3::Distance( { 2, 2, 2 }, { 4, 2, 2 } ) - 2 ) > 0.0001f)
    {
        std::cerr << "Vec3 Distance failed! " << std::endl;
    }
    if (std::abs( Vec3::DistanceSquared( { 2, 2, 2 }, { 4, 2, 2 } ) - 4 ) > 0.0001f)
    {
        std::cerr << "Vec3 DistanceSquared failed! " << std::endl;
    }

    if (!Vec3::Reflect( { 1, 1, 0 }, { 1, 0, 0 } ).IsAlmost( { -1, 1, 0 } ))
    {
        std::cerr << "Vec3 reflect failed! " << std::endl;
    }
}

void TestMatrixTranspose()
{
    Matrix44 matrix;
    const float exceptedResult = 42;
    matrix.m[ 3 ] = exceptedResult;
    matrix.Transpose( matrix );
    
    if (matrix.m[ 3 * 4 ] != exceptedResult)
    {
        std::cerr << "Matrix transpose failed!" << std::endl;
    }
}

void TestMatrixMultiply()
{
    Matrix44 matrix1;
    const float m1data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    matrix1.InitFrom( m1data );

    Matrix44 matrix2;
    const float m2data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    matrix2.InitFrom( m2data );

    Matrix44 result;
    Matrix44::Multiply( matrix1, matrix2, result );

    Matrix44 expectedResult;
    const float exData[] = 
        { 
            90, 100, 110, 120,
            202, 228, 254, 280,
            314, 356, 398, 440,
            426, 484, 542, 600
        };
    expectedResult.InitFrom( exData );

    for (int i = 0; i < 16; ++i)
    {
        if (std::abs( result.m[ i ] - expectedResult.m[ i ] ) > 0.0001f)
        {
            std::cerr << "Matrix multiply failed!" << std::endl;
        }
    }
}

void TestMatrixInverse()
{
    Matrix44 mat;
    mat.MakeProjection( 45, 4.0f / 3.0f, 1, 200 );

    Matrix44 result;
    Matrix44::Invert( mat, result );

    Matrix44 expectedResult;
    const float exData[] = 
    { 
        0.552285f, -0, -0, -0, 
        -0, 0.414214f, -0, -0, 
        -0, -0, -0, -0.4975f, 
        -0, -0, -1, 0.5025f
    };
    expectedResult.InitFrom( exData );

    for (int i = 0; i < 16; ++i)
    {
        if (std::abs( result.m[ i ] - expectedResult.m[ i ] ) > 0.0001f)
        {
            std::cerr << "Matrix inverse failed!" << std::endl;
        }
    }
}

static bool TestQuatConstructor()
{
    const float tx =  1.0f;
    const float ty = -2.5f;
    const float tz =  0.4f;
    const float tw =  2.8f;

    Quaternion q( Vec3( tx, ty, tz ), tw );

    return q.x == tx && q.y == ty && q.z == tz && q.w == tw;
}

static bool TestQuatMultiplyQ1Q2()
{
    const Quaternion q1( Vec3( 2.5f, -1.3f, -5.2f ), 1.0f );
    const Quaternion q2( Vec3( 5.4f,  2.6f,  6.7f ), 1.0f );

    const Quaternion result = q1 * q2;

    const float acceptableDelta = 0.00001f;

    if (result.w - 25.72f > acceptableDelta ||
        result.x - 12.71f > acceptableDelta ||
        result.y + 43.53f > acceptableDelta ||
        result.z - 15.02f > acceptableDelta)
    {
        return false;
    }

    return true;
}

static bool TestQuatNormalize()
{
    Quaternion q( Vec3( 2.5f, -1.3f, -5.2f ), 1.8f );
    q.Normalize();

    return IsAlmost( q.x,  0.404385f ) &&
           IsAlmost( q.y, -0.21028f ) &&
           IsAlmost( q.z, -0.84112f ) &&
           IsAlmost( q.w,  0.291157f );
}

static bool TestQuatGetConjugate()
{
    const float tx = 1.0f;
    const float ty = -2.5f;
    const float tz = 0.4f;
    const float tw = 2.8f;

    const Quaternion q( Vec3( tx, ty, tz ), tw );
    const Quaternion result = q.Conjugate();
    const float acceptableDelta = 0.00001f;

    if (!IsAlmost( -result.x, tx ) ||
        !IsAlmost( -result.y, ty ) ||
        !IsAlmost( -result.z, tz ) ||
        !IsAlmost(  result.w, tw))
    {
        return false;
    }

    return true;
}

bool TestQuatEuler()
{
    // Quaternions to Euler.
    Quaternion q = Quaternion::FromEuler( { 45, 0, 0 } );
    Vec3 euler = q.GetEuler();
    
    const float acceptableDelta = 0.01f;
    const bool result = 45.005f - euler.x < acceptableDelta;

    if (!result)
    {
        std::cerr << "Quaternion::GetEuler failed!" << std::endl;
    }

    return result;
}

void TestQuaternion()
{
    if (!(TestQuatEuler() && TestQuatConstructor() && TestQuatGetConjugate() &&
          TestQuatMultiplyQ1Q2() && TestQuatNormalize()))
    {
        std::cerr << "Quaternion failed!" << std::endl;
    }
}

int main()
{
    TestVec3();
    TestVec4();
    TestMatrixTranspose();
    TestMatrixMultiply();
    TestMatrixInverse();
    TestQuaternion();
}

    
