#include "Renderer.hpp"
#include "FileSystem.hpp"

ae3d::Renderer renderer;

void ae3d::BuiltinShaders::Load()
{
    spriteRendererShader.LoadSPIRV( FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );
    sdfShader.LoadSPIRV( FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );
    skyboxShader.LoadSPIRV( FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );
    momentsShader.LoadSPIRV( FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );
    depthNormalsShader.LoadSPIRV( FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );
}
