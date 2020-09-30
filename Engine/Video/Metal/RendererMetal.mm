#include "Renderer.hpp"
#include "FileSystem.hpp"

ae3d::Renderer renderer;

void ae3d::BuiltinShaders::Load()
{
    spriteRendererShader.LoadFromLibrary( "sprite_vertex", "sprite_fragment" );
    sdfShader.LoadFromLibrary( "sdf_vertex", "sdf_fragment" );
    skyboxShader.LoadFromLibrary( "skybox_vertex", "skybox_fragment" );
    momentsShader.LoadFromLibrary( "moments_vertex", "moments_fragment" );
    momentsSkinShader.LoadFromLibrary( "moments_skin_vertex", "moments_fragment" );
    depthNormalsShader.LoadFromLibrary( "depthnormals_vertex", "depthnormals_fragment" );
    depthNormalsSkinShader.LoadFromLibrary( "depthnormals_skin_vertex", "depthnormals_fragment" );
    lightCullShader.Load( "light_culler", FileSystem::FileContents(""), FileSystem::FileContents("") );
    uiShader.LoadFromLibrary( "sprite_vertex", "sprite_fragment" );
}
