#include "Frustum.hpp"

using namespace ae3d;

void Frustum::UpdateCornersAndCenters( const Vec3& cameraPosition, const Vec3& zAxis )
{
    const Vec3 up( 0, 1, 0 );
    const Vec3 xAxis = Vec3::Cross( up, zAxis ).Normalized();
    const Vec3 yAxis = Vec3::Cross( zAxis, xAxis ).Normalized();
    
    // Computes the centers of near and far planes.
    nearCenter = cameraPosition - zAxis * zNear;
    farCenter  = cameraPosition - zAxis * zFar;
    
    // Computes the near plane corners.
    nearTopLeft     = nearCenter + yAxis * nearHeight - xAxis * nearWidth;
    nearTopRight    = nearCenter + yAxis * nearHeight + xAxis * nearWidth;
    nearBottomLeft  = nearCenter - yAxis * nearHeight - xAxis * nearWidth;
    nearBottomRight = nearCenter - yAxis * nearHeight + xAxis * nearWidth;
    
    // Computes the far plane corners.
    farTopLeft     = farCenter + yAxis * farHeight - xAxis * farWidth;
    farTopRight    = farCenter + yAxis * farHeight + xAxis * farWidth;
    farBottomLeft  = farCenter - yAxis * farHeight - xAxis * farWidth;
    farBottomRight = farCenter - yAxis * farHeight + xAxis * farWidth;
}

float Frustum::NearClipPlane() const
{
    return zNear;
}

float Frustum::FarClipPlane() const
{
    return zFar;
}

Vec3 Frustum::Centroid() const
{
    return (nearCenter + farCenter) * 0.5f;
}

void Frustum::Plane::CalculateNormal()
{
    const Vec3 v1 = a - b;
    const Vec3 v2 = c - b;
    normal = Vec3::Cross( v2, v1 ).Normalized();
    d = -( Vec3::Dot( normal, b ) );
}

void Frustum::Plane::SetNormalAndPoint( const Vec3& aNormal, const Vec3& aPoint )
{
    normal = aNormal.Normalized();
    d = -( Vec3::Dot( normal, aPoint ) );
}

void Frustum::SetProjection( float fieldOfView, float aAspect, float aNear, float aFar )
{
    zNear = aNear;
    zFar  = aFar;
    
    // Computes width and height of the near and far plane sections.
    const float deg2rad = 3.14159265358979f / 180.0f;
    const float tang = tanf( deg2rad * fieldOfView * 0.5f );
    nearHeight = zNear * tang;
    nearWidth  = nearHeight * aAspect;
    farHeight  = zFar * tang;
    farWidth   = farHeight * aAspect;
}

void Frustum::SetProjection( float aLeft, float aRight, float aBottom, float aTop, float aNear, float aFar )
{
    zNear = aNear;
    zFar  = aFar;
    
    nearHeight = aTop - aBottom;
    nearWidth  = aRight - aLeft;
    farHeight  = nearHeight;
    farWidth   = nearWidth;
}

void Frustum::Update( const Vec3& cameraPosition, const Vec3& cameraDirection )
{
    const Vec3 zAxis = cameraDirection;
    UpdateCornersAndCenters( cameraPosition, zAxis );
    
    enum FrustumPlane
    {
        FARP = 0,
        NEARP,
        BOTTOM,
        TOP,
        LEFT,
        RIGHT
    };
    
    planes[ FrustumPlane::TOP ].a = nearTopRight;
    planes[ FrustumPlane::TOP ].b = nearTopLeft;
    planes[ FrustumPlane::TOP ].c = farTopLeft;
    planes[ FrustumPlane::TOP ].CalculateNormal();
    
    planes[ FrustumPlane::BOTTOM ].a = nearBottomLeft;
    planes[ FrustumPlane::BOTTOM ].b = nearBottomRight;
    planes[ FrustumPlane::BOTTOM ].c = farBottomRight;
    planes[ FrustumPlane::BOTTOM ].CalculateNormal();
    
    planes[ FrustumPlane::LEFT ].a = nearTopLeft;
    planes[ FrustumPlane::LEFT ].b = nearBottomLeft;
    planes[ FrustumPlane::LEFT ].c = farBottomLeft;
    planes[ FrustumPlane::LEFT ].CalculateNormal();
    
    planes[ FrustumPlane::RIGHT ].a = nearBottomRight;
    planes[ FrustumPlane::RIGHT ].b = nearTopRight;
    planes[ FrustumPlane::RIGHT ].c = farBottomRight;
    planes[ FrustumPlane::RIGHT ].CalculateNormal();
    
    planes[ FrustumPlane::NEARP ].SetNormalAndPoint( -zAxis, nearCenter );
    planes[ FrustumPlane::FARP  ].SetNormalAndPoint(  zAxis, farCenter  );
}

bool Frustum::BoxInFrustum( const Vec3& min, const Vec3& max ) const
{
    bool result = true;
    
    Vec3 pos;
    //Vec3 neg = max;
    
    // Determines positive and negative vertex in relation to the normal.
    for (unsigned p = 0; p < 6; ++p)
    {
        pos = min;
        //neg = max;
        
        if (planes[ p ].normal.x >= 0)
        {
            pos.x = max.x;
        }
        if (planes[ p ].normal.y >= 0)
        {
            pos.y = max.y;
        }
        if (planes[ p ].normal.z >= 0)
        {
            pos.z = max.z;
        }
        
        /*if (m->planes[ p ].normal.x >= 0)
         {
         neg.x = min.x;
         }
         if (m->planes[ p ].normal.y >= 0)
         {
         neg.y = min.y;
         }
         if (m->planes[ p ].normal.z >= 0)
         {
         neg.z = min.z;
         }*/
        
        if (planes[ p ].Distance( pos ) < 0)
        {
            return false;
        }
    }
    
    return result;
}

const Vec3& Frustum::NearTopLeft() const { return nearTopLeft; }
const Vec3& Frustum::NearTopRight() const { return nearTopRight; }
const Vec3& Frustum::NearBottomLeft() const { return nearBottomLeft; }
const Vec3& Frustum::NearBottomRight() const { return nearBottomRight; }

const Vec3& Frustum::FarTopLeft() const { return farTopLeft; }
const Vec3& Frustum::FarTopRight() const { return farTopRight; }
const Vec3& Frustum::FarBottomLeft() const { return farBottomLeft; }
const Vec3& Frustum::FarBottomRight() const { return farBottomRight; }
