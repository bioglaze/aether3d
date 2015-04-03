#ifndef MATRIX_H
#define MATRIX_H

#include "Macros.hpp"

namespace ae3d
{
    struct Vec3;
    struct Vec4;

    /// 4x4 Matrix.
#if SIMD_SSE3
    struct BEGIN_ALIGNED( Matrix44, 16 )
#else
    struct Matrix44
#endif
    {
        /// Identity matrix.
        static const Matrix44 identity;
        
        /// Converts depth coordinates from [-1,1] to [0,1]. This is needed to sample the shadow map texture.
        static const Matrix44 bias;
        
        /**
         \brief Inverts a matrix.
         
         \param matrix 4x4 matrix.
         \param out Inverted matrix.
         */
        static void Invert( const Matrix44& matrix, Matrix44& out );
        
        /**
         \brief Inverse transpose.
         
         \param m Matrix data.
         \param out Inverse transpose of m.
         \see Transpose.
         */
        static void InverseTranspose( const float m[ 16 ], float* out );
        
        /**
         \param a First Matrix.
         \param b Second Matrix.
         \param out a * b.
         */
        static void Multiply( const Matrix44& a, const Matrix44& b, Matrix44& out );
        
        /**
         Multiplies a matrix with a vector.
         
         \param vec Vector.
         \param mat Matrix.
         \param out vec * mat.
         */
        static void TransformPoint( const Vec4& vec, const Matrix44& mat, Vec4* out );
        
        /**
         Multiplies a matrix with a vector. Vector's missing w is treated as 1.
         
         \param vec Vector.
         \param mat Matrix.
         \param out vec * mat.
         */
        static void TransformPoint( const Vec3& vec, const Matrix44& mat, Vec3* out );
        
        /**
         Multiplies a matrix with a vector. Vector's missing w is treated as 0.
         
         \param dir Direction vector.
         \param mat Matrix.
         \param out dir * mat.
         */
        static void TransformDirection( const Vec3& dir, const Matrix44& mat, Vec3* out );
        
        /** \brief Constructor. Inits to identity. */
        Matrix44()
        {
            MakeIdentity();
        }
        
        /// Constructor that makes an XYZ rotation matrix.
        Matrix44( float xDeg, float yDeg, float zDeg )
        {
            MakeRotationXYZ( xDeg, yDeg, zDeg );
        }
        
        /**
         Copy constructor.
         
         \param other Other matrix.
         */
        Matrix44( const Matrix44& other );
        
        /** \param data Column-major initialization data. */
        explicit Matrix44( const float* data );
        
        /**
         \param other Matrix from which this matrix is initialized.
         \return this.
         */
        Matrix44& operator=( const Matrix44& other )
        {
            InitFrom( other.m );
            return *this;
        }
        
        /**
         \param x X
         \param y Y
         \param z Z
         */
        void Scale( float x, float y, float z );
        
        /// \param data 4x4 column-major matrix data.
        void InitFrom( const float* data );
        
        /// Resets this matrix.
        void MakeIdentity();
        
        /**
         Makes a lookat matrix.
         
         \param eye Eye (camera) position.
         \param center Target position.
         \param up World up vector.
         */
        void MakeLookAt( const Vec3& eye, const Vec3& center, const Vec3& up );
        
        /**
         Sets perspective projection (right-handed).
         Calculated as in gluPerspective (http://www.opengl.org/wiki/GluPerspective_code)
         
         \param fovDegrees Field-of-View in degrees.
         \param aspect Aspect ratio.
         \param nearDepth Near plane distance.
         \param farDepth Far plane distance.
         */
        void MakeProjection( float fovDegrees, float aspect, float nearDepth, float farDepth );
        
        /**
         Sets orthographic projection.
         
         \param left Left.
         \param right Right.
         \param bottom Bottom.
         \param top Top.
         \param nearDepth Near plane distance.
         \param farDepth Far plane distance.
         */
        void MakeProjection( float left, float right, float bottom, float top, float nearDepth, float farDepth );
        
        /**
         Makes a rotation around axes.
         
         \param xDeg X-axis degrees.
         \param yDeg Y-axis degrees.
         \param zDeg Z-axis degrees.
         */
        void MakeRotationXYZ( float xDeg, float yDeg, float zDeg );
        
        /// \param out Result of transpose.
        void Transpose( Matrix44& out ) const;
        
        /// \param v Translation.
        void Translate( const Vec3& v );
        
        /// Member data, row-major.
        float m[16];
    }
#if SIMD_SSE3
    END_ALIGNED( Matrix44, 16 )
#endif
    ;
}
#endif
