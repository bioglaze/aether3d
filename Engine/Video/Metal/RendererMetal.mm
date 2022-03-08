#include "Renderer.hpp"
#include "FileSystem.hpp"
#include "GfxDevice.hpp"
#include "Vec3.hpp"

ae3d::Renderer renderer;

id< MTLBuffer > particleBuffer;
id< MTLBuffer > particleTileBuffer;

void ae3d::BuiltinShaders::Load()
{
    spriteRendererShader.LoadFromLibrary( "sprite_vertex", "sprite_fragment" );
    sdfShader.LoadFromLibrary( "sdf_vertex", "sdf_fragment" );
    skyboxShader.LoadFromLibrary( "skybox_vertex", "skybox_fragment" );
    momentsShader.LoadFromLibrary( "moments_vertex", "moments_fragment" );
    momentsSkinShader.LoadFromLibrary( "moments_skin_vertex", "moments_fragment" );
    momentsAlphaTestShader.LoadFromLibrary( "moments_vertex", "moments_alphatest_fragment" );
    depthNormalsShader.LoadFromLibrary( "depthnormals_vertex", "depthnormals_fragment" );
    depthNormalsSkinShader.LoadFromLibrary( "depthnormals_skin_vertex", "depthnormals_fragment" );
    lightCullShader.Load( "light_culler", FileSystem::FileContents(""), FileSystem::FileContents("") );
    particleSimulationShader.Load( "particle_simulation", FileSystem::FileContents(""), FileSystem::FileContents("") );
    particleDrawShader.Load( "particle_draw", FileSystem::FileContents(""), FileSystem::FileContents("") );
    particleCullShader.Load( "particle_cull", FileSystem::FileContents( "" ), FileSystem::FileContents( "" ) );
    uiShader.LoadFromLibrary( "sprite_vertex", "sprite_fragment" );
    
    const unsigned maxParticleCount = 1000000;
    particleBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:sizeof( Particle ) * maxParticleCount
                              options:MTLResourceStorageModePrivate];
    particleBuffer.label = @"Particle buffer";
    
    const unsigned tileCount = renderer.GetNumParticleTilesX() * renderer.GetNumParticleTilesY();
    const unsigned maxParticlesPerTile = 1000;

    particleTileBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:maxParticlesPerTile * tileCount * sizeof( unsigned )
                  options:MTLResourceStorageModePrivate];
    particleTileBuffer.label = @"particleTileBuffer";
}
