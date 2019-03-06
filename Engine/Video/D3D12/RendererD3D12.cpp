#include "Renderer.hpp"
#include "FileSystem.hpp"
#include "ComputeShader.hpp"

ae3d::Renderer renderer;

void ae3d::BuiltinShaders::Load()
{
    spriteRendererShader.Load( "", "", FileSystem::FileContents( "sprite_vert.obj" ), FileSystem::FileContents( "sprite_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    sdfShader.Load( "", "", FileSystem::FileContents( "sdf_vert.obj" ), FileSystem::FileContents( "sdf_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    skyboxShader.Load( "", "", FileSystem::FileContents( "skybox_vert.obj" ), FileSystem::FileContents( "skybox_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    momentsShader.Load( "", "", FileSystem::FileContents( "moments_vert.obj" ), FileSystem::FileContents( "moments_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    momentsSkinShader.Load( "", "", FileSystem::FileContents( "moments_skin_vert.obj" ), FileSystem::FileContents( "moments_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    depthNormalsShader.Load( "", "", FileSystem::FileContents( "depthnormals_vert.obj" ), FileSystem::FileContents( "depthnormals_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    uiShader.Load( "", "", FileSystem::FileContents( "sprite_vert.obj" ), FileSystem::FileContents( "sprite_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );

    lightCullShader.Load( "", FileSystem::FileContents( "LightCuller.obj" ), FileSystem::FileContents( "" ) );
}
