dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T vs_6_0 hlsl/sprite_vert.hlsl -Fo ../../../aether3d_build/Samples/shaders/sprite_vert.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T ps_6_0 hlsl/sprite_frag.hlsl -Fo ../../../aether3d_build/Samples/shaders/sprite_frag.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T vs_6_0 hlsl/unlit_cube_vert.hlsl -Fo ../../../aether3d_build/Samples/shaders/unlit_cube_vert.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T ps_6_0 hlsl/unlit_cube_frag.hlsl -Fo ../../../aether3d_build/Samples/shaders/unlit_cube_frag.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T vs_6_0 hlsl/unlit_vert.hlsl -Fo ../../../aether3d_build/Samples/shaders/unlit_vert.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T ps_6_0 hlsl/unlit_frag.hlsl -Fo ../../../aether3d_build/Samples/shaders/unlit_frag.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T vs_6_0 hlsl/unlit_skin_vert.hlsl -Fo ../../../aether3d_build/Samples/shaders/unlit_skin_vert.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T vs_6_0 hlsl/moments_vert.hlsl -Fo ../../../aether3d_build/Samples/shaders/moments_vert.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T vs_6_0 hlsl/moments_skin_vert.hlsl -Fo ../../../aether3d_build/Samples/shaders/moments_skin_vert.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T ps_6_0 hlsl/moments_frag.hlsl -Fo ../../../aether3d_build/Samples/shaders/moments_frag.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T vs_6_0 hlsl/skybox_vert.hlsl -Fo ../../../aether3d_build/Samples/shaders/skybox_vert.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T ps_6_0 hlsl/skybox_frag.hlsl -Fo ../../../aether3d_build/Samples/shaders/skybox_frag.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T vs_6_0 hlsl/depthnormals_vert.hlsl -Fo ../../../aether3d_build/Samples/shaders/depthnormals_vert.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T vs_6_0 hlsl/depthnormals_skin_vert.hlsl -Fo ../../../aether3d_build/Samples/shaders/depthnormals_skin_vert.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T ps_6_0 hlsl/depthnormals_frag.hlsl -Fo ../../../aether3d_build/Samples/shaders/depthnormals_frag.spv
dxc -DVULKAN -Ges -spirv -E CSMain -all-resources-bound -T cs_6_0 hlsl/LightCuller.hlsl -Fo ../../../aether3d_build/Samples/shaders/LightCuller.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T vs_6_0 hlsl/Standard_vert.hlsl -Fo ../../../aether3d_build/Samples/shaders/Standard_vert.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T vs_6_0 hlsl/Standard_skin_vert.hlsl -Fo ../../../aether3d_build/Samples/shaders/Standard_skin_vert.spv
dxc -DVULKAN -Ges -spirv -E main -all-resources-bound -T ps_6_0 hlsl/Standard_frag.hlsl -Fo ../../../aether3d_build/Samples/shaders/Standard_frag.spv
dxc -DVULKAN -DENABLE_SHADOWS -Ges -spirv -E main -all-resources-bound -T ps_6_0 hlsl/Standard_frag.hlsl -Fo ../../../aether3d_build/Samples/shaders/Standard_frag_shadow.spv
dxc -DVULKAN -DENABLE_SHADOWS_POINT -Ges -spirv -E main -all-resources-bound -T ps_6_0 hlsl/Standard_frag.hlsl -Fo ../../../aether3d_build/Samples/shaders/Standard_frag_shadow_point.spv
dxc -DVULKAN -Ges -spirv -E CSMain -all-resources-bound -T cs_6_0 hlsl/Bloom.hlsl -Fo ../../../aether3d_build/Samples/shaders/Bloom.spv
dxc -DVULKAN -Ges -spirv -E CSMain -all-resources-bound -T cs_6_0 hlsl/Blur.hlsl -Fo ../../../aether3d_build/Samples/shaders/Blur.spv
dxc -DVULKAN -Ges -spirv -E CSMain -all-resources-bound -T cs_6_0 hlsl/ssao.hlsl -Fo ../../../aether3d_build/Samples/shaders/ssao.spv
dxc -DVULKAN -Ges -spirv -E CSMain -all-resources-bound -T cs_6_0 hlsl/compose.hlsl -Fo ../../../aether3d_build/Samples/shaders/compose.spv
dxc -DVULKAN -Ges -spirv -E CSMain -all-resources-bound -T cs_6_0 hlsl/outline.hlsl -Fo ../../../aether3d_build/Samples/shaders/outline.spv
dxc -DVULKAN -Ges -spirv -E CSMain -all-resources-bound -T cs_6_0 hlsl/particle.hlsl -Fo ../../../aether3d_build/Samples/shaders/particle.spv
dxc -DVULKAN -Ges -spirv -E CSMain -all-resources-bound -T cs_6_0 hlsl/particle_draw.hlsl -Fo ../../../aether3d_build/Samples/shaders/particle_draw.spv

