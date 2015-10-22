#include "VR.hpp"
#include "GameObject.hpp"

#if OCULUS_RIFT

#include "LibOVR/Include/OVR_CAPI_GL.h"
#include "Kernel/OVR_System.h"
#include "LibOVRKernel/Src/GL/CAPI_GLE.h"
#include "LibOVR/Include/Extras/OVR_Math.h"
#include "TransformComponent.hpp"
#include "CameraComponent.hpp"
#include "GfxDevice.hpp"

using namespace ae3d;

namespace GfxDeviceGlobal
{
    extern GLuint systemFBO;
}

struct DepthBuffer
{
    GLuint texId;

    explicit DepthBuffer( OVR::Sizei size )
    {
        glGenTextures( 1, &texId );
        glBindTexture( GL_TEXTURE_2D, texId );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

        GLenum internalFormat = GL_DEPTH_COMPONENT24;
        GLenum type = GL_UNSIGNED_INT;

        if (GLE_ARB_depth_buffer_float)
        {
            internalFormat = GL_DEPTH_COMPONENT32F;
            type = GL_FLOAT;
        }

        glTexImage2D( GL_TEXTURE_2D, 0, internalFormat, size.w, size.h, 0, GL_DEPTH_COMPONENT, type, nullptr );
    }
};

struct TextureBuffer
{
    ovrSwapTextureSet* TextureSet = nullptr;
    GLuint texId;
    GLuint fboId;
    OVR::Sizei texSize;

    TextureBuffer( ovrHmd hmd, bool rendertarget, bool displayableOnHmd, OVR::Sizei size, int mipLevels, unsigned char * data )
    {
        texSize = size;

        if (displayableOnHmd)
        {
            // This texture isn't necessarily going to be a rendertarget, but it usually is.
            OVR_ASSERT( hmd ); // No HMD? A little odd.

            ovr_CreateSwapTextureSetGL( hmd, GL_RGBA, size.w, size.h, &TextureSet );

            for (int i = 0; i < TextureSet->TextureCount; ++i)
            {
                ovrGLTexture* tex = (ovrGLTexture*)&TextureSet->Textures[ i ];
                glBindTexture( GL_TEXTURE_2D, tex->OGL.TexId );

                if (rendertarget)
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
                }
                else
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                }
            }
        }
        else
        {
            glGenTextures( 1, &texId );
            glBindTexture( GL_TEXTURE_2D, texId );

            if (rendertarget)
            {
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
            }
            else
            {
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
            }

            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, texSize.w, texSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
        }

        if (mipLevels > 1)
        {
            glGenerateMipmap( GL_TEXTURE_2D );
        }

        glGenFramebuffers( 1, &fboId );
    }

    OVR::Sizei GetSize( void ) const
    {
        return texSize;
    }

    void SetAndClearRenderSurface( DepthBuffer * dbuffer )
    {
        GfxDeviceGlobal::systemFBO = fboId;

        ovrGLTexture* tex = (ovrGLTexture*)&TextureSet->Textures[ TextureSet->CurrentIndex ];

        glBindFramebuffer( GL_FRAMEBUFFER, fboId );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->OGL.TexId, 0 );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dbuffer->texId, 0 );

        glViewport( 0, 0, texSize.w, texSize.h );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }

    void UnsetRenderSurface()
    {
        glBindFramebuffer( GL_FRAMEBUFFER, fboId );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0 );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0 );
    }
};

namespace Global
{
    OVR::GLEContext GLEContext;
    ovrEyeRenderDesc EyeRenderDesc[ 2 ];
    TextureBuffer* eyeRenderTexture[ 2 ] = {};
    DepthBuffer* eyeDepthBuffer[ 2 ] = {};
    ovrHmd hmd;
    ovrHmdDesc HMD;
    ovrGLTexture* mirrorTexture = nullptr;
    GLuint mirrorFBO = 0;
    ovrPosef EyeRenderPose[ 2 ];
    ovrVector3f ViewOffset[ 2 ];
}

void ae3d::VR::Init()
{
    OVR::System::Init();

    if (ovr_Initialize( nullptr ) != ovrSuccess)
    {
        MessageBoxA( nullptr, "Unable to initialize libOVR.", "", MB_OK );
        return;
    }

    ovrGraphicsLuid luid;
    ovrResult result = ovr_Create( &Global::hmd, &luid );

    if (!OVR_SUCCESS( result ))
    {
        MessageBoxA( nullptr, "Oculus Rift not detected.", "", MB_OK );
        ovr_Shutdown();
        return;
    }

    Global::HMD = ovr_GetHmdDesc( Global::hmd );

    if (Global::HMD.ProductName[ 0 ] == '\0')
    {
        MessageBoxA( nullptr, "Rift detected, display not enabled.", "", MB_OK );
    }
}

void ae3d::VR::GetIdealWindowSize( int& outWidth, int& outHeight )
{
   const ovrSizei idealTextureSize = ovr_GetFovTextureSize( Global::hmd, (ovrEyeType)0, Global::HMD.DefaultEyeFov[ 0 ], 1 );
   outWidth = idealTextureSize.w;
   outHeight = idealTextureSize.h;
}

void ae3d::VR::StartTracking( int windowWidth, int windowHeight )
{
    OVR::GLEContext::SetCurrentContext( &Global::GLEContext );
    Global::GLEContext.Init();

    for (int i = 0; i < 2; ++i)
    {
        ovrSizei idealTextureSize;
        idealTextureSize.w = windowWidth;
        idealTextureSize.h = windowHeight;
        const bool isRenderTarget = true;

        Global::eyeRenderTexture[ i ] = new TextureBuffer( Global::hmd, isRenderTarget, true, idealTextureSize, 1, nullptr );
        Global::eyeDepthBuffer[ i ] = new DepthBuffer( Global::eyeRenderTexture[ i ]->GetSize() );
    }

    ovr_CreateMirrorTextureGL( Global::hmd, GL_RGBA, windowWidth, windowHeight, (ovrTexture**)&Global::mirrorTexture );
    glGenFramebuffers( 1, &Global::mirrorFBO );
    glBindFramebuffer( GL_READ_FRAMEBUFFER, Global::mirrorFBO );
    glFramebufferTexture2D( GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Global::mirrorTexture->OGL.TexId, 0 );
    glFramebufferRenderbuffer( GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0 );
    glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );

    Global::EyeRenderDesc[ 0 ] = ovr_GetRenderDesc( Global::hmd, ovrEye_Left, Global::HMD.DefaultEyeFov[ 0 ] );
    Global::EyeRenderDesc[ 1 ] = ovr_GetRenderDesc( Global::hmd, ovrEye_Right, Global::HMD.DefaultEyeFov[ 1 ] );

    ovr_SetEnabledCaps( Global::hmd, 0 );

    wglSwapIntervalEXT( 0 );
}

void ae3d::VR::CalcEyePose()
{
    Global::ViewOffset[ 0 ] = Global::EyeRenderDesc[ 0 ].HmdToEyeViewOffset;
    Global::ViewOffset[ 1 ] = Global::EyeRenderDesc[ 1 ].HmdToEyeViewOffset;

    double ftiming = ovr_GetPredictedDisplayTime( Global::hmd, 0 );
    ovrTrackingState hmdState = ovr_GetTrackingState( Global::hmd, ftiming, ovrTrue );
    ovr_CalcEyePoses( hmdState.HeadPose.ThePose, Global::ViewOffset, Global::EyeRenderPose );
}

void ae3d::VR::SetEye( int eye )
{
    Global::eyeRenderTexture[ eye ]->TextureSet->CurrentIndex = (Global::eyeRenderTexture[ eye ]->TextureSet->CurrentIndex + 1) % Global::eyeRenderTexture[ eye ]->TextureSet->TextureCount;
    Global::eyeRenderTexture[ eye ]->SetAndClearRenderSurface( Global::eyeDepthBuffer[ eye ] );
}

void ae3d::VR::UnsetEye( int eye )
{
    Global::eyeRenderTexture[ eye ]->UnsetRenderSurface();
}

void ae3d::VR::SubmitFrame()
{
    ovrViewScaleDesc viewScaleDesc;
    viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
    viewScaleDesc.HmdToEyeViewOffset[ 0 ] = Global::ViewOffset[ 0 ];
    viewScaleDesc.HmdToEyeViewOffset[ 1 ] = Global::ViewOffset[ 1 ];

    ovrLayerEyeFov ld;
    ld.Header.Type = ovrLayerType_EyeFov;
    ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft | ovrLayerFlag_HighQuality;

    for (int eye = 0; eye < 2; ++eye)
    {
        ld.ColorTexture[ eye ] = Global::eyeRenderTexture[ eye ]->TextureSet;
        ld.Viewport[ eye ] = OVR::Recti( Global::eyeRenderTexture[ eye ]->GetSize() );
        ld.Fov[ eye ] = Global::HMD.DefaultEyeFov[ eye ];
        ld.RenderPose[ eye ] = Global::EyeRenderPose[ eye ];
    }

    ovrLayerHeader* layers = &ld.Header;
    /*ovrResult result = */ovr_SubmitFrame( Global::hmd, 0, &viewScaleDesc, &layers, 1 );
    //isVisible = OVR_SUCCESS( result );

    // Blit mirror texture to back buffer
    glBindFramebuffer( GL_READ_FRAMEBUFFER, Global::mirrorFBO );
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
    const GLint w = Global::mirrorTexture->OGL.Header.TextureSize.w;
    const GLint h = Global::mirrorTexture->OGL.Header.TextureSize.h;
    glBlitFramebuffer( 0, h, w, 0, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST );
    glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
}

void ae3d::VR::CalcCameraForEye( GameObject& camera, float yawDegrees, int eye )
{
    if (!camera.GetComponent< TransformComponent >() || !camera.GetComponent< CameraComponent >())
    {
        return;
    }

    const auto& cameraPosition = camera.GetComponent< TransformComponent >()->GetLocalPosition();
    const auto cameraPosOVR = OVR::Vector3f( cameraPosition.x, cameraPosition.y, cameraPosition.z );

    OVR::Matrix4f rollPitchYaw = OVR::Matrix4f::RotationY( yawDegrees * 3.14159265f / 180.0f );
    OVR::Matrix4f finalRollPitchYaw = rollPitchYaw * OVR::Matrix4f( Global::EyeRenderPose[ eye ].Orientation );
    OVR::Vector3f finalUp = finalRollPitchYaw.Transform( OVR::Vector3f( 0, 1, 0 ) );
    OVR::Vector3f finalForward = finalRollPitchYaw.Transform( OVR::Vector3f( 0, 0, -1 ) );
    OVR::Vector3f shiftedEyePos = cameraPosOVR + rollPitchYaw.Transform( Global::EyeRenderPose[ eye ].Position );

    OVR::Matrix4f view = OVR::Matrix4f::LookAtRH( shiftedEyePos, shiftedEyePos + finalForward, finalUp );
    OVR::Matrix4f proj = ovrMatrix4f_Projection( Global::HMD.DefaultEyeFov[ eye ], 0.2f, 1000.0f, ovrProjection_RightHanded );

    Matrix44 viewMat;
    view.Transpose();
    viewMat.InitFrom( (float*)view.M );
    Matrix44 projMat;
    proj.Transpose();
    projMat.InitFrom( (float*)proj.M );

    camera.GetComponent< TransformComponent >()->SetVrView( viewMat );
    camera.GetComponent< CameraComponent >()->SetProjection( projMat );
}

#else

using namespace ae3d;

void ae3d::VR::Init()
{
}

void ae3d::VR::GetIdealWindowSize( int& outWidth, int& outHeight )
{
    outWidth = 640;
    outHeight = 480;
}

void ae3d::VR::StartTracking( int /*windowWidth*/, int /*windowHeight*/ )
{
}

void ae3d::VR::SubmitFrame()
{
}

void ae3d::VR::SetEye( int /*eye*/ )
{
}

void ae3d::VR::UnsetEye( int /*eye*/ )
{
}

void ae3d::VR::CalcEyePose()
{
}

void ae3d::VR::CalcCameraForEye( GameObject& /*camera*/, float /* yawDegrees */, int /*eye*/ )
{
}

#endif