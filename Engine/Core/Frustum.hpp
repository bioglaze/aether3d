#ifndef FRUSTUM_H
#define FRUSTUM_H

#include "Vec3.hpp"

namespace ae3d
{
/**
 View Frustum.
 
 Contains the view frustum methods used for frustum culling.
 All coordinates/tests are done in world space.
 Frustum plane normals point towards the inside of the frustum.
 */
class Frustum
{
public:
    const Vec3& NearTopLeft() const;
    const Vec3& NearTopRight() const;
    const Vec3& NearBottomLeft() const;
    const Vec3& NearBottomRight() const;
    
    const Vec3& FarTopLeft() const;
    const Vec3& FarTopRight() const;
    const Vec3& FarBottomLeft() const;
    const Vec3& FarBottomRight() const;
    
    /**
     Tests AABB against the frustum.
     
     \param min AABB's minimum corner.
     \param max AABB's maximum corner.
     \return True, if part of the box is in the frustum.
     \return False, if the box is not in the frustum.
     */
    bool BoxInFrustum( const Vec3& min, const Vec3& max ) const;
    
    /**
     Sets values from which the frustum is calculated.
     Should be called when the perspective is changed.
     The values should be the same as in gluPerspective()
     
     \param fieldOfView Vertical field of view in degrees
     \param aaspect Aspect ratio (width / height)
     \param anear Near clipping plane z-coordinate
     \param afar Far clipping plane z-coordinate
     */
    void SetProjection( float fieldOfView, float aspect, float anear, float afar );
    
    /**
     Sets orthographic projection.
     
     \param left Left.
     \param right Right.
     \param bottom Bottom.
     \param top Top.
     \param nearDepth Near plane distance.
     \param farDepth Far plane distance.
     */
    void SetProjection( float left, float right, float bottom, float top, float nearp, float farp );
    
    /**
     Calculates the frustum planes. Should be called when the camera's origin
     or direction changes.
     
     \param cameraPosition Camera's position
     \param cameraDirection Camera's direction
     */
    void Update( const Vec3& cameraPosition, const Vec3& cameraDirection );
    
    /// \return Near clip plane.
    float NearClipPlane() const;
    
    /// \return Far clip plane.
    float FarClipPlane() const;
    
    /// \return Centroid.
    Vec3 Centroid() const;
    
private:
    void UpdateCornersAndCenters( const Vec3& cameraPosition, const Vec3& zAxis );
    
    // Near clipping plane coordinates
    Vec3 nearTopLeft;
    Vec3 nearTopRight;
    Vec3 nearBottomLeft;
    Vec3 nearBottomRight;
    
    // Far clipping plane coordinates.
    Vec3 farTopLeft;
    Vec3 farTopRight;
    Vec3 farBottomLeft;
    Vec3 farBottomRight;
    
    Vec3 nearCenter; // Near clipping plane center coordinate.
    Vec3 farCenter;  // Far clipping plane center coordinate.
    float zNear, zFar; // The same as in gluPerspective()
    float nearWidth, nearHeight;
    float farWidth, farHeight;
    
    /**
     3 points define a plane. The frustum has 6 planes.
     */
    struct Plane
    {
        /**
         \param point Point to be tested.
         \return point's distance to the plane.
         */
        float Distance( const Vec3& point ) const
        {
            return Vec3::Dot( normal, point ) + d;
        }
        
        /**
         Sets plane's normal and point.
         
         \param normal Normal.
         \param point Point.
         */
        void SetNormalAndPoint( const Vec3& normal, const Vec3& point );
        
        /// Calculates plane's normal.
        void CalculateNormal();
        
        Vec3 a; // Point a on the plane.
        Vec3 b; // Point b on the plane.
        Vec3 c; // Point c on the plane.
        Vec3 normal; // Plane's normal pointing inside the frustum.
        float d;
    } planes[ 6 ]; // Clipping planes.
};
}
#endif

