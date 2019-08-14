#include <cmath>
#include "CppUnitTest.h"
#include "Array.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "Matrix.hpp"
#include "Quaternion.hpp"
#include "Scene.hpp"
#include "Vec3.hpp"

using namespace ae3d;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

bool IsAlmost( float f1, float f2 )
{
    const float tolerance = 0.0001f;
    return std::abs( f1 - f2 ) < tolerance;
}

namespace UnitTests
{		
    TEST_CLASS(MathTests)
    {
    public:
		
        TEST_METHOD( Vec4Constructor )
        {
            Assert::IsTrue( Vec4( 1, 2, 3, 4 ).IsAlmost( Vec4( 1, 2, 3, 4 ) ) );
        }

        TEST_METHOD( Vec4DefaultConstructor )
        {
            Vec4 vec1;
            Assert::IsTrue( vec1.IsAlmost( Vec4( 0, 0, 0, 0 ) ) );
        }

        TEST_METHOD( Vec4FromVec3 )
        {
            Vec4 vec2( { 1, 2, 3 } );

            Assert::IsTrue( vec2.IsAlmost( Vec4( 1, 2, 3, 1 ) ) );
        }

        TEST_METHOD( Vec3Constructor )
        {
            Vec3 vec;
            Assert::IsTrue( vec.IsAlmost( Vec3( 0, 0, 0 ) ) );
        }

        TEST_METHOD( Vec3Addition )
        {
            Vec3 vec1 = Vec3( 2, 3, 2 );
            Vec3 vec2( 3, 2, 3 );

            Assert::IsTrue( (vec1 + vec2).IsAlmost( Vec3( 5, 5, 5 ) ) );
        }

        TEST_METHOD( Vec3Subtract )
        {
            Vec3 vec1 = Vec3( 2, 3, 2 );
            Vec3 vec2( 3, 2, 3 );

            Assert::IsTrue( (vec1 - vec2).IsAlmost( Vec3( -1, 1, -1 ) ) );
        }

        TEST_METHOD( Vec3Multiply )
        {
            Vec3 vec1 = Vec3( 2, 3, 2 );
            Vec3 vec2( 3, 2, 3 );

            Assert::IsTrue( (vec1 * vec2).IsAlmost( Vec3( 6, 6, 6 ) ) );
        }

        TEST_METHOD( Vec3Normalize )
        {
            Vec3 vec2( 3, 2, 3 );
            vec2 = vec2.Normalized();

            Assert::IsFalse( std::abs( vec2.Length() - 1 ) > 0.0001f );
        }

        TEST_METHOD( Vec3Division )
        {
            Assert::IsTrue( (Vec3( 1, 2, 3 ) / 2.0f).IsAlmost( Vec3( 0.5f, 1.0f, 1.5f ) ) );
        }

        TEST_METHOD( Vec3MultiplyByScalar )
        {
            Assert::IsTrue( (Vec3( 1, 2, 3 ) * 2.0f).IsAlmost( Vec3( 2.0f, 4.0f, 6.0f ) )  );
        }

        TEST_METHOD( Vec3Assign )
        {
            Vec3 a( 1, 2, 3 );
            Vec3 b = a;

            Assert::IsTrue( a.IsAlmost( b ) );
        }

        TEST_METHOD( Vec3Assign2 )
        {
            Assert::IsTrue( Vec3{ 3, 3, 3 }.IsAlmost( { 3, 3, 3 } ) );
        }

        TEST_METHOD( Vec3Misc )
        {
            Vec3 a( 1, 2, 3 );
            Vec3 b = Vec3( 1, 1, 1 );
            Vec3 r = a + b - b;
            Assert::IsTrue( r.IsAlmost( a ) );
        }

        TEST_METHOD( Vec3Misc2 )
        {
            Vec3 a( 1, 2, 3 );
            Vec3 b = Vec3( 2, 2, 2 );
            Vec3 r = a / b;

            Assert::IsTrue( (r * b).IsAlmost( a ) );
        }

        TEST_METHOD( Vec3Min )
        {
            Assert::IsTrue( Vec3::Min2( { 1, 2, 3 }, { 3, 1, 2 } ).IsAlmost( { 1, 1, 2 } ) );
        }

        TEST_METHOD( Vec3Max )
        {
            Assert::IsTrue( Vec3::Max2( { 1, 2, 3 }, { 3, 1, 2 } ).IsAlmost( { 3, 2, 3 } ) );
        }

        TEST_METHOD( Vec3Dot )
        {
            Assert::IsTrue( std::abs( Vec3::Dot( { 2, 2, 2 }, { 2, 2, 2 } ) - 12 ) < 0.0001f );
        }

        TEST_METHOD( Vec3Cross )
        {
            Assert::IsTrue( Vec3::Cross( { 1, 0, 0 }, { 0, 1, 0 } ).IsAlmost( { 0, 0, 1 } ) );
        }

        TEST_METHOD( Vec3Distance )
        {
            Assert::IsTrue( std::abs( Vec3::Distance( { 2, 2, 2 }, { 4, 2, 2 } ) - 2 ) < 0.0001f );
        }

        TEST_METHOD( Vec3DistanceSquared )
        {
            Assert::IsTrue( std::abs( Vec3::DistanceSquared( { 2, 2, 2 }, { 4, 2, 2 } ) - 4 ) < 0.0001f );
        }

        TEST_METHOD( Vec3Reflect )
        {
            Assert::IsTrue( Vec3::Reflect( { 1, 1, 0 }, { 1, 0, 0 } ).IsAlmost( { -1, 1, 0 } ) );
        }

        TEST_METHOD( MatrixTranspose )
        {
            Matrix44 matrix;
            const float exceptedResult = 42;
            matrix.m[ 3 ] = exceptedResult;
            matrix.Transpose( matrix );

            Assert::IsTrue( matrix.m[ 3 * 4 ] == exceptedResult );
        }

        TEST_METHOD( MatrixMultiply )
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

            bool success = true;
            for (int i = 0; i < 16; ++i)
            {
                if (std::abs( result.m[ i ] - expectedResult.m[ i ] ) > 0.0001f)
                {
                    success = false;
                }
            }

            Assert::IsTrue( success );
        }

        TEST_METHOD( MatrixInverse )
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

            bool success = true;

            for (int i = 0; i < 16; ++i)
            {
                if (std::abs( result.m[ i ] - expectedResult.m[ i ] ) > 0.0001f)
                {
                    success = false;
                }
            }

            Assert::IsTrue( success );
        }

        TEST_METHOD( QuaternionConstructor )
        {
            const float tx = 1.0f;
            const float ty = -2.5f;
            const float tz = 0.4f;
            const float tw = 2.8f;

            Quaternion q( Vec3( tx, ty, tz ), tw );

            Assert::IsTrue( q.x == tx && q.y == ty && q.z == tz && q.w == tw );
        }

        TEST_METHOD( QuaternionMultiplyQ1Q2 )
        {
            const Quaternion q1( Vec3( 2.5f, -1.3f, -5.2f ), 1.0f );
            const Quaternion q2( Vec3( 5.4f, 2.6f, 6.7f ), 1.0f );

            const Quaternion result = q1 * q2;

            const float acceptableDelta = 0.00001f;

            Assert::IsFalse( result.w - 25.72f > acceptableDelta ||
                result.x - 12.71f > acceptableDelta ||
                result.y + 43.53f > acceptableDelta ||
                result.z - 15.02f > acceptableDelta );
        }

        TEST_METHOD( QuaternionNormalize )
        {
            Quaternion q( Vec3( 2.5f, -1.3f, -5.2f ), 1.8f );
            q.Normalize();

            Assert::IsTrue( IsAlmost( q.x, 0.404385f ) &&
                            IsAlmost( q.y, -0.21028f ) &&
                            IsAlmost( q.z, -0.84112f ) &&
                            IsAlmost( q.w, 0.291157f ) );
        }

        TEST_METHOD( QuaternionConjugate )
        {
            const float tx = 1.0f;
            const float ty = -2.5f;
            const float tz = 0.4f;
            const float tw = 2.8f;

            const Quaternion q( Vec3( tx, ty, tz ), tw );
            const Quaternion result = q.Conjugate();

            Assert::IsTrue( IsAlmost( -result.x, tx ) &&
                IsAlmost( -result.y, ty ) &&
                IsAlmost( -result.z, tz ) &&
                IsAlmost( result.w, tw ) );
        }

        TEST_METHOD( QuaternionToEuler )
        {
            Quaternion q = Quaternion::FromEuler( { 45, 0, 0 } );
            Vec3 euler = q.GetEuler();

            const float acceptableDelta = 0.01f;
            Assert::IsTrue( 45.005f - euler.x < acceptableDelta );
        }

        TEST_METHOD( FileSystemNullPath )
        {
            auto contents = FileSystem::FileContents( nullptr );
            Assert::IsFalse( contents.isLoaded );
        }
    
        TEST_METHOD( FileSystemNotFoundPath )
        {
            auto contents = FileSystem::FileContents( "can't find me" );
            Assert::IsFalse( contents.isLoaded );
        }

        TEST_METHOD( SceneSerialization )
        {
            Scene scene;
            GameObject go;
            go.SetName( "my game object" );
            scene.Add( &go );

            auto serialized = scene.GetSerialized();

            Assert::IsTrue( serialized.find( "my game object" ) != std::string::npos );
        }

        TEST_METHOD( ArrayTest1 )
        {
            Array< int > arr( 1 );
            arr.Remove( 0 );

            Assert::IsTrue( arr.count == 0 );
        }

        TEST_METHOD( ArrayTest2 )
        {
            Array< int > arr( 0 );
            arr.Remove( 0 );

            Assert::IsTrue( true );
        }

        TEST_METHOD( ArrayTest3 )
        {
            Array< int > arr1( 10 );
            Array< int > arr2( 5 );
            arr1 = arr2;

            Assert::IsTrue( arr1.count == 5 );
        }

        TEST_METHOD( ArrayTest4 )
        {
            Array< int > arr1( 10 );
            arr1[ 0 ] = 666;
            Array< int > arr2( 5 );
            arr2 = arr1;

            Assert::IsTrue( arr2[ 0 ] == 666 );
        }

    };
}