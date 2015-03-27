#ifndef VEC3_H
#define VEC3_H

#include <cmath>

namespace ae3d
{
    /**
    \brief
    3-component vector.
    */
    struct Vec3
    {
        float x = 0, y = 0, z = 0;
        
        /** Copy constructor. */
        Vec3(const Vec3& other) = default;

        /**
           \brief Cross product.

           \param v1 Vector.
           \param v2 Another vector.
           \return Cross product of this vector and v.
           */
        static Vec3 Cross(const Vec3& v1, const Vec3& v2)
        {
            return Vec3(v1.y * v2.z - v1.z * v2.y,
                v1.z * v2.x - v1.x * v2.z,
                v1.x * v2.y - v1.y * v2.x);
        }

        /**
         Solves distance between two vectors.

         \param v1 Vector.
         \param v2 Other vector.
         \return Distance.
         \see DistanceSquared()
         */
        static float Distance(const Vec3& v1, const Vec3& v2)
        {
            return (v2 - v1).Length();
        }

        /**
         Solves squared distance between two vectors.

         \param v1 Vector.
         \param v2 Other vector.
         \return Squared distance.
         \see Distance()
         */
        static float DistanceSquared(const Vec3& v1, const Vec3& v2)
        {
            const Vec3 sub = v2 - v1;
            return sub.x * sub.x + sub.y * sub.y + sub.z * sub.z;
        }

        /**
         Dot product.

         \param v1 Vector.
         \param v2 Other vector.
         \return Dot product of vectors v1 and v2.
         */
        static float Dot(const Vec3& v1, const Vec3& v2)
        {
            return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
        }

        /**
         Reflects a vector with normal.

         \param vec Vector.
         \param normal Normal.
         \return Reflected vector.
         */
        static Vec3 Reflect(const Vec3& vec, const Vec3& normal) { return vec - normal * Dot(normal, vec) * 2; }

        // Default constructor
        Vec3() {}

        /**
         Constructor.

         \param ax X-coordinate.
         \param ay Y-coordinate.
         \param az Z-coordinate.
         */
        Vec3(float ax, float ay, float az) : x(ax), y(ay), z(az) {}

        /**
           Divides by a scalar operator.

           \param f Scalar value.
           \return a vector that has its values divided by f.
           */
        Vec3 operator/(float f) const
        {
            const float inv = 1.0f / f;
            return Vec3(x * inv, y * inv, z * inv);
        }

        /**
           Divides by a vector operator.

           \param v Vector value.
           \return a vector that has its values divided by v.
           */
        Vec3 operator/(const Vec3& v) const
        {
            return Vec3(x / v.x, y / v.y, z / v.z);
        }

        /**
           Negation operator.

           \return a vector that has negative values.
           */
        Vec3 operator-() const
        {
            return Vec3(-x, -y, -z);
        }

        /**
           Subtraction operator.

           \param v a vector that is to be subtracted from this vector.
           \return result of the subtraction.
           */
        Vec3 operator-(const Vec3& v) const
        {
            return Vec3(x - v.x, y - v.y, z - v.z);
        }

        /**
           Addition operator.

           \param v a vector that is to be added to this vector.
           \return A vector that is the sum of this vector and v.
           */
        Vec3 operator+(const Vec3& v) const
        {
            return Vec3(x + v.x, y + v.y, z + v.z);
        }

        /**
           Addition operator.

           \param f a value that is to be added to this vector.
           \return A vector that is the sum of this vector and c.
           */
        Vec3 operator+(float f) const
        {
            return Vec3(x + f, y + f, z + f);
        }

        /**
           Operator subtract and assign.

           \param v Vector that is to be subtracted from this vector.
           \return Reference to this vector.
           */
        Vec3& operator-=(const Vec3& v)
        {
            x -= v.x;
            y -= v.y;
            z -= v.z;

            return *this;
        }

        /**
         Operator add and assign.

         \param v Vector that is to be added to this vector.
         \return Reference to this vector.
         */
        Vec3& operator+=(const Vec3& v)
        {
            x += v.x;
            y += v.y;
            z += v.z;

            return *this;
        }

        /**
           Operator multiply and assign.

           \param f A value that is to be multiplied with this vector.
           \return Reference to this vector.
           */
        Vec3& operator*=(float f)
        {
            x *= f;
            y *= f;
            z *= f;

            return *this;
        }

        /**
        Operator divide and assign.

        \param f A value that is to be divided with this vector.
        \return Reference to this vector.
        */
        Vec3& operator/=(float f)
        {
            const float inv = 1.0f / f;

            x *= inv;
            y *= inv;
            z *= inv;

            return *this;
        }

        /**
           Operator multiply and assign.

           \param v A value that is to be multiplied with this vector.
           \return Reference to this vector.
           */
        Vec3& operator*=(const Vec3& v)
        {
            x *= v.x;
            y *= v.y;
            z *= v.z;

            return *this;
        }

        /**
           Assignment operator.

           \param v a vector that is assigned to this vector.
           \return Reference to this vector.
           */
        Vec3& operator=(const Vec3& v) { x = v.x; y = v.y; z = v.z; return *this; }

        /**
           Multiplication operator.

           \param v A vector that is multiplied with this vector
           \return A vector that is the product of this vector and v.
           */
        Vec3 operator*(const Vec3& v) const
        {
            return Vec3(x * v.x, y * v.y, z * v.z);
        }

        /**
           Multiplication operator.

           \param f A value that is multiplied with this vector.
           \return This vector multiplied by f.
           */
        Vec3 operator*(float f) const
        {
            return Vec3(x * f, y * f, z * f);
        }

        /**
           Returns a normalized vector
           */
        Vec3 Normalized() const
        {
            const float len = Length();

            if (std::abs(len) < 0.0001f)
            {
                return Vec3(1, 0, 0);
            }

            Vec3 out = *this * (1.0f / len);
            return out;
        }

        /// \return length.
        float Length() const { return std::sqrt(x * x + y * y + z * z); }

        /// Resets this vector's values to 0.
        void Zero() { x = y = z = 0; }
    };

    /**
     4-component vector.
     */
    struct Vec4
    {
        float x = 0, y = 0, z = 0, w = 0;

        Vec4() {}
        
        /**
         Constructor.

         \param ax X-coordinate.
         \param ay Y-coordinate.
         \param az Z-coordinate.
         \param aw W-coordinate.
         */
        Vec4(float ax, float ay, float az, float aw) : x(ax), y(ay), z(az), w(aw) {}

        /// \param v Vec3
        Vec4(const Vec3& v) : x(v.x), y(v.y), z(v.z), w(1) {}

        /**
           Constructor.

           \param v Vec3
           \param aW .w component
           */
        Vec4(const Vec3& v, float aW) : x(v.x), y(v.y), z(v.z), w(aW) {}

        /**
         Multiplication operator.

         \param f A value that is multiplied with this vector.
         \return This vector multiplied by f.
         */
        Vec4 operator*(float f) const
        {
            return Vec4(x * f, y * f, z * f, w * f);
        }

        /**
         Operator addition and assign.

         \param v Vector that is to be added to this vector.
         */
        Vec4& operator+=(const Vec4& v)
        {
            x += v.x;
            y += v.y;
            z += v.z;

            return *this;
        }

        /**
         Operator subtract and assign.

         \param v Vector that is to be subtracted from this vector.
         */
        Vec4& operator-=(const Vec4& v)
        {
            x -= v.x;
            y -= v.y;
            z -= v.z;

            return *this;
        }

        /**
         Dot product.

         \param v another vector.
         \return Dot product of this vector and v.
         */
        float Dot(const Vec4& v) const
        {
            return x * v.x + y * v.y + z * v.z + w * v.w;
        }

        /// \return length.
        float Length() const { return std::sqrt(x * x + y * y + z * z); }

        /**
         Normalizes this vector.
         */
        void Normalize()
        {
            const float len = Length();

            if (std::abs(len) < 0.0001f)
            {
                x = 1;
                y = 0;
                z = 0;
                return;
            }

            const float iLength = 1 / len;

            x *= iLength;
            y *= iLength;
            z *= iLength;
        }
    };
}
#endif
