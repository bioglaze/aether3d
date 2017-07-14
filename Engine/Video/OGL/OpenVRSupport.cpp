#if AE3D_OPENVR
#include "VR.hpp"
#include <cstdint>
#include <cmath>
#include <memory>
#include <string>
#include <vector>
#include <openvr.h>
#include <GL/glxw.h>
#include "CameraComponent.hpp"
#include "GameObject.hpp"
#include "Matrix.hpp"
#include "System.hpp"
#include "Shader.hpp"
#include "TransformComponent.hpp"
#include "Vec3.hpp"

using namespace ae3d;
extern unsigned boundTextures[ 16 ];

struct FramebufferDesc
{
    GLuint depthBufferId;
    GLuint renderTextureId;
    GLuint renderFramebufferId;
    GLuint resolveTextureId;
    GLuint resolveFramebufferId;
};

struct Vector2
{
    Vector2() : x( 0 ), y( 0 ) {}
    Vector2( float aX, float aY ) : x( aX ), y( aY ) {}

    float x, y;
};

struct VertexDataLens
{
    Vector2 position;
    Vector2 texCoordRed;
    Vector2 texCoordGreen;
    Vector2 texCoordBlue;
};

namespace WindowGlobal
{
    extern int windowWidth;
    extern int windowHeight;
}

namespace Global
{
    extern GLuint activeVao;

    vr::IVRSystem* hmd = nullptr;
    vr::IVRRenderModels* renderModels = nullptr;
    std::uint32_t width, height;
    FramebufferDesc leftEyeDesc;
    FramebufferDesc rightEyeDesc;
    GLuint idVertexBuffer;
    GLuint idIndexBuffer;
    GLuint lensVAO;
    unsigned int indexSize;
    float vrFov = 45;
    vr::TrackedDevicePose_t trackedDevicePose[ vr::k_unMaxTrackedDeviceCount ];
    int validPoseCount;
    Matrix44 mat4HMDPose;
    Matrix44 eyePosLeft;
    Matrix44 eyePosRight;
    std::string poseClasses;
    char devClassChar[ vr::k_unMaxTrackedDeviceCount ];   // for each device, a character representing its class
    Matrix44 devicePose[ vr::k_unMaxTrackedDeviceCount ];
    Shader lensDistort;
    Vec3 vrEyePosition; // Used in Scene.cpp for frustum culling
    Vec3 leftControllerPosition;
    Vec3 rightControllerPosition;
    int leftHandIndex = -1;
    int rightHandIndex = -1;
}

float GetVRFov()
{
    return Global::vrFov;
}

bool CreateFrameBuffer( int width, int height, FramebufferDesc& framebufferDesc )
{
    glGenFramebuffers( 1, &framebufferDesc.renderFramebufferId );
    glBindFramebuffer( GL_FRAMEBUFFER, framebufferDesc.renderFramebufferId );

    glGenRenderbuffers( 1, &framebufferDesc.depthBufferId );
    glBindRenderbuffer( GL_RENDERBUFFER, framebufferDesc.depthBufferId );
    glRenderbufferStorageMultisample( GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, width, height );
    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.depthBufferId );

    glGenTextures( 1, &framebufferDesc.renderTextureId );
    glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.renderTextureId );
    glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, width, height, true );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.renderTextureId, 0 );

    glGenFramebuffers( 1, &framebufferDesc.resolveFramebufferId );
    glBindFramebuffer( GL_FRAMEBUFFER, framebufferDesc.resolveFramebufferId );

    glGenTextures( 1, &framebufferDesc.resolveTextureId );
    glBindTexture( GL_TEXTURE_2D, framebufferDesc.resolveTextureId );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.resolveTextureId, 0 );

    GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        System::Print( "OpenVR: Unable to create framebuffer!" );
        return false;
    }

    glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    return true;
}

void SetupDistortion()
{
    const GLushort lensGridSegmentCountH = 43;
    const GLushort lensGridSegmentCountV = 43;

    const float w = (float)(1.0 / float( lensGridSegmentCountH - 1 ));
    const float h = (float)(1.0 / float( lensGridSegmentCountV - 1 ));

    float u, v = 0;

    std::vector< VertexDataLens > vVerts( 0 );
    VertexDataLens vert;

    // Left eye distortion verts
    float Xoffset = -1;
    for (int y = 0; y < lensGridSegmentCountV; ++y)
    {
        for (int x = 0; x < lensGridSegmentCountH; ++x)
        {
            u = x * w; v = 1 - y * h;
            vert.position = Vector2( Xoffset + u, -1 + 2 * y*h );

            vr::DistortionCoordinates_t dc0;
            const bool success = Global::hmd->ComputeDistortion( vr::Eye_Left, u, v, &dc0 );

            vert.texCoordRed = Vector2( dc0.rfRed[ 0 ], 1 - dc0.rfRed[ 1 ] );
            vert.texCoordGreen = Vector2( dc0.rfGreen[ 0 ], 1 - dc0.rfGreen[ 1 ] );
            vert.texCoordBlue = Vector2( dc0.rfBlue[ 0 ], 1 - dc0.rfBlue[ 1 ] );

            vVerts.push_back( vert );
        }
    }

    // Right eye distortion verts
    Xoffset = 0;
    for (int y = 0; y < lensGridSegmentCountV; ++y)
    {
        for (int x = 0; x < lensGridSegmentCountH; ++x)
        {
            u = x * w; v = 1 - y * h;
            vert.position = Vector2( Xoffset + u, -1 + 2 * y * h );

            vr::DistortionCoordinates_t dc0;
            const bool success = Global::hmd->ComputeDistortion( vr::Eye_Right, u, v, &dc0 );

            vert.texCoordRed = Vector2( dc0.rfRed[ 0 ], 1 - dc0.rfRed[ 1 ] );
            vert.texCoordGreen = Vector2( dc0.rfGreen[ 0 ], 1 - dc0.rfGreen[ 1 ] );
            vert.texCoordBlue = Vector2( dc0.rfBlue[ 0 ], 1 - dc0.rfBlue[ 1 ] );

            vVerts.push_back( vert );
        }
    }

    std::vector<GLushort> vIndices;
    GLushort a, b, c, d;

    GLushort offset = 0;
    for (GLushort y = 0; y < lensGridSegmentCountV - 1; ++y)
    {
        for (GLushort x = 0; x < lensGridSegmentCountH - 1; ++x)
        {
            a = lensGridSegmentCountH * y + x + offset;
            b = lensGridSegmentCountH * y + x + 1 + offset;
            c = (y + 1) * lensGridSegmentCountH + x + 1 + offset;
            d = (y + 1) * lensGridSegmentCountH + x + offset;
            vIndices.push_back( a );
            vIndices.push_back( b );
            vIndices.push_back( c );

            vIndices.push_back( a );
            vIndices.push_back( c );
            vIndices.push_back( d );
        }
    }

    offset = lensGridSegmentCountH * lensGridSegmentCountV;
    for (GLushort y = 0; y < lensGridSegmentCountV - 1; ++y)
    {
        for (GLushort x = 0; x < lensGridSegmentCountH - 1; ++x)
        {
            a = lensGridSegmentCountH * y + x + offset;
            b = lensGridSegmentCountH * y + x + 1 + offset;
            c = (y + 1) * lensGridSegmentCountH + x + 1 + offset;
            d = (y + 1) * lensGridSegmentCountH + x + offset;
            vIndices.push_back( a );
            vIndices.push_back( b );
            vIndices.push_back( c );

            vIndices.push_back( a );
            vIndices.push_back( c );
            vIndices.push_back( d );
        }
    }
    
    Global::indexSize = static_cast< unsigned int >( vIndices.size() );

    glGenVertexArrays( 1, &Global::lensVAO );
    glBindVertexArray( Global::lensVAO );
    Global::activeVao = Global::lensVAO;

    glGenBuffers( 1, &Global::idVertexBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, Global::idVertexBuffer );
    glBufferData( GL_ARRAY_BUFFER, vVerts.size() * sizeof( VertexDataLens ), vVerts.data(), GL_STATIC_DRAW );

    glGenBuffers( 1, &Global::idIndexBuffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, Global::idIndexBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, vIndices.size() * sizeof( GLushort ), vIndices.data(), GL_STATIC_DRAW );

    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( VertexDataLens ), (void *)offsetof( VertexDataLens, position ) );

    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( VertexDataLens ), (void *)offsetof( VertexDataLens, texCoordRed ) );

    glEnableVertexAttribArray( 2 );
    glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( VertexDataLens ), (void *)offsetof( VertexDataLens, texCoordGreen ) );

    glEnableVertexAttribArray( 3 );
    glVertexAttribPointer( 3, 2, GL_FLOAT, GL_FALSE, sizeof( VertexDataLens ), (void *)offsetof( VertexDataLens, texCoordBlue ) );

    glBindVertexArray( 0 );
    Global::activeVao = 0;

    glDisableVertexAttribArray( 0 );
    glDisableVertexAttribArray( 1 );
    glDisableVertexAttribArray( 2 );
    glDisableVertexAttribArray( 3 );

    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

std::string GetTrackedDeviceString( vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL )
{
    std::uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty( unDevice, prop, NULL, 0, peError );
    if (unRequiredBufferLen == 0)
    {
        return "";
    }

    char* pchBuffer = new char[ unRequiredBufferLen ];
    unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty( unDevice, prop, pchBuffer, unRequiredBufferLen, peError );
    std::string sResult = pchBuffer;
    delete[] pchBuffer;
    return sResult;
}

void ConvertSteamVRMatrixToMatrix4( const vr::HmdMatrix34_t &steamMatrix, Matrix44& outMatrix )
{
    outMatrix.m[ 0 ] = steamMatrix.m[ 0 ][ 0 ];
    outMatrix.m[ 1 ] = steamMatrix.m[ 0 ][ 1 ];
    outMatrix.m[ 2 ] = steamMatrix.m[ 0 ][ 2 ];
    outMatrix.m[ 3 ] = steamMatrix.m[ 0 ][ 3 ];
    outMatrix.m[ 4 ] = steamMatrix.m[ 1 ][ 0 ];
    outMatrix.m[ 5 ] = steamMatrix.m[ 1 ][ 1 ];
    outMatrix.m[ 6 ] = steamMatrix.m[ 1 ][ 2 ];
    outMatrix.m[ 7 ] = steamMatrix.m[ 1 ][ 3 ];
    outMatrix.m[ 8 ] = steamMatrix.m[ 2 ][ 0 ];
    outMatrix.m[ 9 ] = steamMatrix.m[ 2 ][ 1 ];
    outMatrix.m[10 ] = steamMatrix.m[ 2 ][ 2 ];
    outMatrix.m[11 ] = steamMatrix.m[ 2 ][ 3 ];
    outMatrix.m[12 ] = 0;
    outMatrix.m[13 ] = 0;
    outMatrix.m[14 ] = 0;
    outMatrix.m[15 ] = 1;

    outMatrix.Transpose( outMatrix );
}

void RenderDistortion()
{
    System::Assert( Global::lensDistort.IsValid(), "lens distortion shader is not valid" );

    glDisable( GL_CULL_FACE ); // glaze test

    glDisable( GL_DEPTH_TEST );
    glViewport( 0, 0, Global::width, Global::height );

    glBindVertexArray( Global::lensVAO );
    Global::activeVao = Global::lensVAO;
    Global::lensDistort.Use();

    glProgramUniform1i( Global::lensDistort.GetHandle(), glGetUniformLocation( Global::lensDistort.GetHandle(), "mytexture" ), 0 );

    //render left lens (first half of index array )
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, Global::leftEyeDesc.resolveTextureId );
    boundTextures[ 0 ] = Global::leftEyeDesc.resolveTextureId;

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glDrawElements( GL_TRIANGLES, Global::indexSize / 2, GL_UNSIGNED_SHORT, 0 );

    //render right lens (second half of index array )
    boundTextures[ 0 ] = Global::rightEyeDesc.resolveTextureId;
    glBindTexture( GL_TEXTURE_2D, Global::rightEyeDesc.resolveTextureId );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glDrawElements( GL_TRIANGLES, Global::indexSize / 2, GL_UNSIGNED_SHORT, (const void *)(Global::indexSize) );

    glBindVertexArray( 0 );
    Global::activeVao = 0;
}

void CompileShaders()
{
    const char* lensVertexSource =
        "#version 330 core\n"
        "layout(location = 0) in vec4 position;\n"
        "layout(location = 1) in vec2 v2UVredIn;\n"
        "layout(location = 2) in vec2 v2UVGreenIn;\n"
        "layout(location = 3) in vec2 v2UVblueIn;\n"
        "noperspective out vec2 v2UVred;\n"
        "noperspective out vec2 v2UVgreen;\n"
        "noperspective out vec2 v2UVblue;\n"
        "void main()\n"
        "{\n"
        "	v2UVred = v2UVredIn;\n"
        "	v2UVgreen = v2UVGreenIn;\n"
        "	v2UVblue = v2UVblueIn;\n"
        "	gl_Position = position;\n"
        "}\n";

    const char* lensFragmentSource =
        "#version 330 core\n"
        "uniform sampler2D mytexture;\n"

        "noperspective in vec2 v2UVred;\n"
        "noperspective in vec2 v2UVgreen;\n"
        "noperspective in vec2 v2UVblue;\n"

        "out vec4 outputColor;\n"

        "void main()\n"
        "{\n"
        "	float fBoundsCheck = ( (dot( vec2( lessThan( v2UVgreen.xy, vec2(0.05, 0.05)) ), vec2(1.0, 1.0))+dot( vec2( greaterThan( v2UVgreen.xy, vec2( 0.95, 0.95)) ), vec2(1.0, 1.0))) );\n"
        "	if( fBoundsCheck > 1.0 )\n"
        "	{ outputColor = vec4( 0, 0, 0, 1.0 ); }\n"
        "	else\n"
        "	{\n"
        "		float red = texture(mytexture, v2UVred).x;\n"
        "		float green = texture(mytexture, v2UVgreen).y;\n"
        "		float blue = texture(mytexture, v2UVblue).z;\n"
        "		outputColor = vec4( red, green, blue, 1.0  ); }\n"
        "}\n";

    Global::lensDistort.Load( lensVertexSource, lensFragmentSource );
}

Vec3 ae3d::VR::GetRightHandPosition()
{
    return Global::rightControllerPosition;
}

Vec3 ae3d::VR::GetLeftHandPosition()
{
    return Global::leftControllerPosition;
}

void ae3d::VR::Init()
{
    vr::EVRInitError eError = vr::VRInitError_None;
    Global::hmd = vr::VR_Init( &eError, vr::VRApplication_Scene );

    if (eError != vr::VRInitError_None)
    {
        System::Print( "Unable to init OpenVR: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription( eError ) );
        return;
    }

    Global::renderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, &eError );

    if (eError != vr::VRInitError_None)
    {
        System::Print( "Unable to get OpenVR render models: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription( eError ) );
    }

    std::string driver = GetTrackedDeviceString( Global::hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String );
    std::string display = GetTrackedDeviceString( Global::hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String );
    System::Print( "OpenVR driver: %s, display: %s\n", driver.c_str(), display.c_str() );

    Global::hmd->GetRecommendedRenderTargetSize( &Global::width, &Global::height );
    System::Print( "OpenVR width: %d, height: %d\n", Global::width, Global::height );

    CreateFrameBuffer( Global::width, Global::height, Global::leftEyeDesc );
    CreateFrameBuffer( Global::width, Global::height, Global::rightEyeDesc );

    CompileShaders();
    SetupDistortion();

    if (Global::hmd)
    {
        vr::HmdMatrix34_t matEye = Global::hmd->GetEyeToHeadTransform( vr::Eye_Left );
        ConvertSteamVRMatrixToMatrix4( matEye, Global::eyePosLeft );
        
        matEye = Global::hmd->GetEyeToHeadTransform( vr::Eye_Right );
        ConvertSteamVRMatrixToMatrix4( matEye, Global::eyePosRight );
    }
}

void ae3d::VR::Deinit()
{
    System::Print("window size: %d x %d\n", WindowGlobal::windowWidth, WindowGlobal::windowHeight );

    if (Global::hmd)
    {
        vr::VR_Shutdown();
        Global::hmd = nullptr;
    }
}

void ae3d::VR::GetIdealWindowSize( int& outWidth, int& outHeight )
{
    if (!Global::hmd)
    {
        return;
    }

    std::uint32_t width, height;
    Global::hmd->GetRecommendedRenderTargetSize( &width, &height );

    outWidth = static_cast< int >( width );
    outHeight = static_cast< int >( height );
}

void ae3d::VR::StartTracking( int /*windowWidth*/, int /*windowHeight*/ )
{
}

void ae3d::VR::RecenterTracking()
{
}

void ae3d::VR::SubmitFrame()
{
    if (!Global::hmd)
    {
        return;
    }

    for (vr::TrackedDeviceIndex_t controllerIndex = 0; controllerIndex < vr::k_unMaxTrackedDeviceCount; ++controllerIndex)
    {
        vr::VRControllerState_t state;

        // button 2 is the one above the "trackpad"
        if (Global::hmd->GetControllerState( controllerIndex, &state, sizeof( state ) ))
        {
            //System::Print( "button state for controller %d: %ld\n", controllerIndex, state.ulButtonPressed );
            //m_rbShowTrackedDevice[ unDevice ] = state.ulButtonPressed == 0;
        }
    }

    RenderDistortion();

    vr::Texture_t leftEyeTexture = { (void*)Global::leftEyeDesc.resolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
    vr::EVRCompositorError submitResult = vr::VRCompositor()->Submit( vr::Eye_Left, &leftEyeTexture );

    if (submitResult != vr::VRCompositorError_None)
    {
        System::Print( "VR submit for left eye returned error %d\n", submitResult );
    }

    vr::Texture_t rightEyeTexture = { (void*)Global::rightEyeDesc.resolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
    
    submitResult = vr::VRCompositor()->Submit( vr::Eye_Right, &rightEyeTexture );
    
    if (submitResult != vr::VRCompositorError_None)
    {
        System::Print( "VR submit for right eye returned error %d\n", submitResult );
    }

    vr::VRCompositor()->PostPresentHandoff();
}

void ae3d::VR::SetEye( int eye )
{
    if (!Global::hmd)
    {
        return;
    }

    if (eye == 0)
    {
        glClearColor( 0.15f, 0.15f, 0.58f, 1.0f );
    }
    if (eye == 1)
    {
        glClearColor( 0.15f, 0.55f, 0.18f, 1.0f );
    }

    glEnable( GL_MULTISAMPLE );

    const FramebufferDesc& fbDesc = eye == 0 ? Global::leftEyeDesc : Global::rightEyeDesc;
    glBindFramebuffer( GL_FRAMEBUFFER, fbDesc.renderFramebufferId );
    glViewport( 0, 0, Global::width, Global::height );
}

void ae3d::VR::UnsetEye( int eye )
{
    if (!Global::hmd)
    {
        return;
    }
    
    glDisable( GL_MULTISAMPLE );

    const FramebufferDesc& fbDesc = eye == 0 ? Global::leftEyeDesc : Global::rightEyeDesc;

    glBindFramebuffer( GL_READ_FRAMEBUFFER, fbDesc.renderFramebufferId );
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, fbDesc.resolveFramebufferId );

    glBlitFramebuffer( 0, 0, Global::width, Global::height, 0, 0, Global::width, Global::height, GL_COLOR_BUFFER_BIT, GL_LINEAR );

    glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );

    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void ae3d::VR::CalcEyePose()
{
    if (!Global::hmd)
    {
        return;
    }

    vr::VRCompositor()->WaitGetPoses( Global::trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0 );

    Global::validPoseCount = 0;
    Global::poseClasses = "";

    for (int deviceIndex = 0; deviceIndex < vr::k_unMaxTrackedDeviceCount; ++deviceIndex)
    {
        if (Global::trackedDevicePose[ deviceIndex ].bPoseIsValid)
        {
            ++Global::validPoseCount;
            ConvertSteamVRMatrixToMatrix4( Global::trackedDevicePose[ deviceIndex ].mDeviceToAbsoluteTracking, Global::devicePose[ deviceIndex ] );
            
            if (Global::devClassChar[ deviceIndex ] == 0)
            {
                switch (Global::hmd->GetTrackedDeviceClass( deviceIndex ))
                {
                case vr::TrackedDeviceClass_Controller:        Global::devClassChar[ deviceIndex ] = 'C'; break;
                case vr::TrackedDeviceClass_HMD:               Global::devClassChar[ deviceIndex ] = 'H'; break;
                case vr::TrackedDeviceClass_Invalid:           Global::devClassChar[ deviceIndex ] = 'I'; break;
                case vr::TrackedDeviceClass_GenericTracker:    Global::devClassChar[ deviceIndex ] = 'O'; break;
                case vr::TrackedDeviceClass_TrackingReference: Global::devClassChar[ deviceIndex ] = 'T'; break;
                default:                                       Global::devClassChar[ deviceIndex ] = '?'; break;
                }
            }

            Global::poseClasses += Global::devClassChar[ deviceIndex ];
        }
    }

    for (int deviceIndex = 0; deviceIndex < vr::k_unMaxTrackedDeviceCount; ++deviceIndex)
    {
        if (Global::devClassChar[ deviceIndex ] == 'C' && Global::leftHandIndex == -1)
        {
            Global::leftHandIndex = deviceIndex;
        }
        else if (Global::devClassChar[ deviceIndex ] == 'C' && Global::rightHandIndex == -1)
        {
            Global::rightHandIndex = deviceIndex;
        }
    }

    if (Global::rightHandIndex != -1 && Global::trackedDevicePose[ Global::rightHandIndex ].bPoseIsValid)
    {
        Global::rightControllerPosition = Vec3( Global::devicePose[ Global::rightHandIndex ].m[ 12 ],
                                                Global::devicePose[ Global::rightHandIndex ].m[ 13 ],
                                                Global::devicePose[ Global::rightHandIndex ].m[ 14 ] );
        //System::Print( "right hand position: %f %f %f\n", Global::rightControllerPosition.x, Global::rightControllerPosition.y, Global::rightControllerPosition.z );
    }

    if (Global::leftHandIndex != -1 && Global::trackedDevicePose[ Global::leftHandIndex ].bPoseIsValid)
    {
        Global::leftControllerPosition = Vec3( Global::devicePose[ Global::leftHandIndex ].m[ 12 ],
                                               Global::devicePose[ Global::leftHandIndex ].m[ 13 ],
                                               Global::devicePose[ Global::leftHandIndex ].m[ 14 ] );
    }

    if (Global::trackedDevicePose[ vr::k_unTrackedDeviceIndex_Hmd ].bPoseIsValid)
    {
        Global::vrEyePosition = Vec3( Global::devicePose[ vr::k_unTrackedDeviceIndex_Hmd ].m[ 12 ],
                                      Global::devicePose[ vr::k_unTrackedDeviceIndex_Hmd ].m[ 13 ],
                                      Global::devicePose[ vr::k_unTrackedDeviceIndex_Hmd ].m[ 14 ] );
        Matrix44::Invert( Global::devicePose[ vr::k_unTrackedDeviceIndex_Hmd ], Global::mat4HMDPose );
    }
}

void ae3d::VR::CalcCameraForEye( GameObject& camera, float /*yawDegrees*/, int eye )
{
    if (!Global::hmd || !camera.GetComponent< TransformComponent >() || !camera.GetComponent< CameraComponent >())
    {
        return;
    }

    const float nearPlane = camera.GetComponent< CameraComponent >()->GetNear();
    const float farPlane = camera.GetComponent< CameraComponent >()->GetFar();

    vr::HmdMatrix44_t mat = Global::hmd->GetProjectionMatrix( eye == 0 ? vr::Hmd_Eye::Eye_Left : vr::Hmd_Eye::Eye_Right, nearPlane, farPlane );
    Matrix44 projMat;
    std::memcpy( &projMat.m[ 0 ], &mat.m[ 0 ][ 0 ], sizeof( Matrix44 ) );
    projMat.Transpose( projMat );

    Matrix44 view;
    Matrix44::Multiply( Global::mat4HMDPose, eye == 0 ? Global::eyePosLeft : Global::eyePosRight, view );
    camera.GetComponent< TransformComponent >()->SetVrView( view );
    camera.GetComponent< CameraComponent >()->SetProjection( projMat );

    Global::vrFov = float( std::atan( Global::height / nearPlane ) ) * (180.0f / 3.14159265f);
    //System::Print("vrFor: %f\n", Global::vrFov );
}
#endif
