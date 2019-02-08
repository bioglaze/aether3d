#include "CameraComponent.hpp"
#include <locale>
#include <sstream>

static constexpr int MaxComponents = 20;
ae3d::CameraComponent cameraComponents[ MaxComponents ];
unsigned nextFreeCameraComponent = 0;

namespace GfxDeviceGlobal
{
    extern unsigned backBufferWidth;
    extern unsigned backBufferHeight;
}

unsigned ae3d::CameraComponent::New()
{
    if (nextFreeCameraComponent == MaxComponents - 1)
    {
        return nextFreeCameraComponent;
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

std::string GetSerialized( ae3d::CameraComponent* component )
{
    std::stringstream outStream;
    std::locale c_locale( "C" );
    outStream.imbue( c_locale );

    outStream << "camera\n";

    outStream << "ortho " << component->GetLeft() << " " << component->GetRight() << " " << component->GetTop() << " " << component->GetBottom() <<
    " " << component->GetNear() << " " << component->GetFar() << "\n";

    outStream << "projection ";
    
    if (component->GetProjectionType() == ae3d::CameraComponent::ProjectionType::Perspective)
    {
        outStream << "perspective\n";
    }
    else
    {
        outStream << "orthographic\n";
    }
    
    outStream << "persp " << component->GetFovDegrees() << " " << component->GetAspect() << " " << component->GetNear() << " " << component->GetFar() << "\n";
    outStream << "layermask " << component->GetLayerMask() << "\n";
    outStream << "order " << component->GetRenderOrder() << "\n";
    outStream << "viewport " << component->GetViewport()[ 0 ] << " " << component->GetViewport()[ 1 ] << " " << component->GetViewport()[ 2 ] << " " << component->GetViewport()[ 3 ] << "\n";
    outStream << "clearcolor " << component->GetClearColor().x << " " << component->GetClearColor().y << " " << component->GetClearColor().z;
    outStream << "\nenabled " << component->IsEnabled() << "\n";
    outStream << "\n\n";

    return outStream.str();
}
