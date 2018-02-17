#include "Renderer.hpp"
#include "FileSystem.hpp"
#include "ComputeShader.hpp"

ae3d::Renderer renderer;
void CreateComputePSO( ae3d::ComputeShader& shader );
void CreateFullscreenTrianglePSO( ID3DBlob* vertexBlob, ID3DBlob* pixelBlob );

void ae3d::BuiltinShaders::Load()
{
    spriteRendererShader.Load( FileSystem::FileContents( "" ), FileSystem::FileContents( "" ), "", "", FileSystem::FileContents( "sprite_vert.hlsl" ), FileSystem::FileContents( "sprite_frag.hlsl" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    sdfShader.Load( FileSystem::FileContents( "" ), FileSystem::FileContents( "" ), "", "", FileSystem::FileContents( "sdf_vert.hlsl" ), FileSystem::FileContents( "sdf_frag.hlsl" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    skyboxShader.Load( FileSystem::FileContents( "" ), FileSystem::FileContents( "" ), "", "", FileSystem::FileContents( "skybox_vert.hlsl" ), FileSystem::FileContents( "skybox_frag.hlsl" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    momentsShader.Load( FileSystem::FileContents( "" ), FileSystem::FileContents( "" ), "", "", FileSystem::FileContents( "moments_vert.hlsl" ), FileSystem::FileContents( "moments_frag.hlsl" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    depthNormalsShader.Load( FileSystem::FileContents( "" ), FileSystem::FileContents( "" ), "", "", FileSystem::FileContents( "depthnormals_vert.hlsl" ), FileSystem::FileContents( "depthnormals_frag.hlsl" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    uiShader.Load( FileSystem::FileContents( "" ), FileSystem::FileContents( "" ), "", "", FileSystem::FileContents( "sprite_vert.hlsl" ), FileSystem::FileContents( "sprite_frag.hlsl" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    fullscreenTriangleShader.Load( FileSystem::FileContents( "" ), FileSystem::FileContents( "" ), "", "", FileSystem::FileContents( "fullscreen_triangle_vert.hlsl" ), FileSystem::FileContents( "sprite_frag.hlsl" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    CreateFullscreenTrianglePSO( fullscreenTriangleShader.blobShaderVertex, fullscreenTriangleShader.blobShaderPixel );

    lightCullShader.Load( "", FileSystem::FileContents( "LightCuller.hlsl" ), FileSystem::FileContents( "" ) );
    CreateComputePSO( lightCullShader );
}
