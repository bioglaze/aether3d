#include "Renderer.hpp"
#include "FileSystem.hpp"

ae3d::Renderer renderer;

void ae3d::BuiltinShaders::Load()
{
    spriteRendererShader.LoadSPIRV( FileSystem::FileContents( "shaders/sprite_vert.spv" ), FileSystem::FileContents( "shaders/sprite_frag.spv" ) );
    sdfShader.LoadSPIRV( FileSystem::FileContents( "shaders/sprite_vert.spv" ), FileSystem::FileContents( "shaders/sprite_frag.spv" ) );
    skyboxShader.LoadSPIRV( FileSystem::FileContents( "shaders/skybox_vert.spv" ), FileSystem::FileContents( "shaders/skybox_frag.spv" ) );
    momentsShader.LoadSPIRV( FileSystem::FileContents( "shaders/moments_vert.spv" ), FileSystem::FileContents( "shaders/moments_frag.spv" ) );
    momentsSkinShader.LoadSPIRV( FileSystem::FileContents( "shaders/moments_skin_vert.spv" ), FileSystem::FileContents( "shaders/moments_frag.spv" ) );
    depthNormalsShader.LoadSPIRV( FileSystem::FileContents( "shaders/depthnormals_vert.spv" ), FileSystem::FileContents( "shaders/depthnormals_frag.spv" ) );
    depthNormalsSkinShader.LoadSPIRV( FileSystem::FileContents( "shaders/depthnormals_skin_vert.spv" ), FileSystem::FileContents( "shaders/depthnormals_frag.spv" ) );
    uiShader.LoadSPIRV( FileSystem::FileContents( "shaders/sprite_vert.spv" ), FileSystem::FileContents( "shaders/sprite_frag.spv" ) );
    lightCullShader.LoadSPIRV( FileSystem::FileContents( "shaders/LightCuller.spv" ) );
}
