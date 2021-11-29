#include "Renderer.hpp"
#include "FileSystem.hpp"
#include "ComputeShader.hpp"

ae3d::Renderer renderer;

void ae3d::BuiltinShaders::Load()
{
    spriteRendererShader.Load( "", "", FileSystem::FileContents( "shaders/sprite_vert.obj" ), FileSystem::FileContents( "shaders/sprite_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    sdfShader.Load( "", "", FileSystem::FileContents( "shaders/sdf_vert.obj" ), FileSystem::FileContents( "shaders/sdf_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    skyboxShader.Load( "", "", FileSystem::FileContents( "shaders/skybox_vert.obj" ), FileSystem::FileContents( "shaders/skybox_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    momentsShader.Load( "", "", FileSystem::FileContents( "shaders/moments_vert.obj" ), FileSystem::FileContents( "shaders/moments_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    momentsSkinShader.Load( "", "", FileSystem::FileContents( "shaders/moments_skin_vert.obj" ), FileSystem::FileContents( "shaders/moments_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    depthNormalsShader.Load( "", "", FileSystem::FileContents( "shaders/depthnormals_vert.obj" ), FileSystem::FileContents( "shaders/depthnormals_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    depthNormalsSkinShader.Load( "", "", FileSystem::FileContents( "shaders/depthnormals_skin_vert.obj" ), FileSystem::FileContents( "shaders/depthnormals_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    uiShader.Load( "", "", FileSystem::FileContents( "shaders/sprite_vert.obj" ), FileSystem::FileContents( "shaders/sprite_frag.obj" ), FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );

    lightCullShader.Load( "", FileSystem::FileContents( "shaders/LightCuller.obj" ), FileSystem::FileContents( "" ) );
    particleCullShader.Load( "", FileSystem::FileContents( "shaders/particle_cull.obj" ), FileSystem::FileContents( "" ) );
    particleSimulationShader.Load( "", FileSystem::FileContents( "shaders/particle_simulate.obj" ), FileSystem::FileContents( "" ) );
    particleDrawShader.Load( "", FileSystem::FileContents( "shaders/particle_draw.obj" ), FileSystem::FileContents( "" ) );
}
