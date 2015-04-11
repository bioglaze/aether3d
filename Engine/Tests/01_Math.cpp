#include <iostream>
#include "Vec3.hpp"
#include "Matrix.hpp"

using namespace ae3d;

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

int main()
{
    TestVec3();    
    TestMatrixTranspose();
    TestMatrixMultiply();
    TestMatrixInverse();
}

    
