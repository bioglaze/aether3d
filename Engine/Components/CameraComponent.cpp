#include "CameraComponent.hpp"
#include <vector>
#include <locale>
#include <sstream>

std::vector< ae3d::CameraComponent > cameraComponents;
unsigned nextFreeCameraComponent = 0;

namespace GfxDeviceGlobal
{
    extern int backBufferWidth;
    extern int backBufferHeight;
}

unsigned ae3d::CameraComponent::New()
{
    if (nextFreeCameraComponent == cameraComponents.size())
    {
        cameraComponents.resize( cameraComponents.size() + 10 );
    }

    cameraComponents[ nextFreeCameraComponent ].viewport[ 0 ] = 0;
    cameraComponents[ nextFreeCameraComponent ].viewport[ 1 ] = 0;
#if RENDERER_METAL
    cameraComponents[ nextFreeCameraComponent ].viewport[ 2 ] = GfxDeviceGlobal::backBufferWidth * 2;
    cameraComponents[ nextFreeCameraComponent ].viewport[ 3 ] = GfxDeviceGlobal::backBufferHeight * 2;
#else
    cameraComponents[ nextFreeCameraComponent ].viewport[ 2 ] = GfxDeviceGlobal::backBufferWidth;
    cameraComponents[ nextFreeCameraComponent ].viewport[ 3 ] = GfxDeviceGlobal::backBufferHeight;
#endif
    return nextFreeCameraComponent++;
}

ae3d::CameraComponent* ae3d::CameraComponent::Get( unsigned index )
{
    return &cameraComponents[ index ];
}

ae3d::Vec3 ae3d::CameraComponent::GetScreenPoint( const ae3d::Vec3 &worldPoint, float viewWidth, float viewHeight ) const
{
    Matrix44 worldToClip;
    Matrix44::Multiply( worldToView, viewToClip, worldToClip );
    alignas( 16 ) Vec4 pos( worldPoint );
    Matrix44::TransformPoint( pos, worldToClip, &pos );
    pos.x /= pos.w;
    pos.y /= pos.w;
    
    const float halfWidth = viewWidth * 0.5f;
    const float halfHeight = viewHeight * 0.5f;
    
    return Vec3( pos.x * halfWidth + halfWidth, -pos.y * halfHeight + halfHeight, pos.z );
}

void ae3d::CameraComponent::SetProjection( float left, float right, float bottom, float top, float aNear, float aFar )
{
    orthoParams.left = left;
    orthoParams.right = right;
    orthoParams.top = top;
    orthoParams.down = bottom;
    nearp = aNear;
    farp = aFar;
    viewToClip.MakeProjection( orthoParams.left, orthoParams.right, orthoParams.down, orthoParams.top, nearp, farp );
}

void ae3d::CameraComponent::SetProjection( float aFovDegrees, float aAspect, float aNear, float aFar )
{
    nearp = aNear;
    farp = aFar;
    fovDegrees = aFovDegrees;
    aspect = aAspect;
    viewToClip.MakeProjection( fovDegrees, aspect, aNear, aFar );
}

void ae3d::CameraComponent::SetProjection( const Matrix44& proj )
{
    viewToClip = proj;
}

void ae3d::CameraComponent::SetClearColor( const Vec3& color )
{
    clearColor = color;
}

void ae3d::CameraComponent::SetTargetTexture( ae3d::RenderTexture* renderTexture )
{
    targetTexture = renderTexture;
}

void ae3d::CameraComponent::SetViewport( int x, int y, int width, int height )
{
    viewport[ 0 ] = x;
    viewport[ 1 ] = y;
    viewport[ 2 ] = width;
    viewport[ 3 ] = height;
}

std::string ae3d::CameraComponent::GetSerialized() const
{
    std::stringstream outStream;
    std::locale c_locale( "C" );
    outStream.imbue( c_locale );

    outStream << "camera\n";

    outStream << "ortho " << orthoParams.left << " " << orthoParams.right << " " << orthoParams.top << " " << orthoParams.down <<
    " " << nearp << " " << farp << "\n";

    outStream << "projection ";
    
    if (projectionType == ProjectionType::Perspective)
    {
        outStream << "perspective\n";
    }
    else
    {
        outStream << "orthographic\n";
    }
    
    outStream << "persp " << fovDegrees << " " << aspect << " " << nearp << " " << farp << "\n";
    outStream << "layermask " << layerMask << "\n";
    outStream << "order " << renderOrder << "\n";
    outStream << "viewport " << viewport[ 0 ] << " " << viewport[ 1 ] << " " << viewport[ 2 ] << " " << viewport[ 3 ] << "\n";
    outStream << "clearcolor " << clearColor.x << " " << clearColor.y << " " << clearColor.z;
    outStream << "enabled" << isEnabled << "\n";
    outStream << "\n\n";

    return outStream.str();
}
