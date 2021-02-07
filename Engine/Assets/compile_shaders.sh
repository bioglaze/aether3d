glslangValidator -D -V -S vert -e main hlsl/sprite_vert.hlsl -o ../../../aether3d_build/Samples/shaders/sprite_vert.spv
glslangValidator -D -V -S frag -e main hlsl/sprite_frag.hlsl -o ../../../aether3d_build/Samples/shaders/sprite_frag.spv
glslangValidator -D -V -S vert -e main hlsl/unlit_cube_vert.hlsl -o ../../../aether3d_build/Samples/shaders/unlit_cube_vert.spv
glslangValidator -D -V -S frag -e main hlsl/unlit_cube_frag.hlsl -o ../../../aether3d_build/Samples/shaders/unlit_cube_frag.spv
glslangValidator -D -V -S vert -e main hlsl/unlit_vert.hlsl -o ../../../aether3d_build/Samples/shaders/unlit_vert.spv
glslangValidator -D -V -S frag -e main hlsl/unlit_frag.hlsl -o ../../../aether3d_build/Samples/shaders/unlit_frag.spv
glslangValidator -D -V -S vert -e main hlsl/unlit_skin_vert.hlsl -o ../../../aether3d_build/Samples/shaders/unlit_skin_vert.spv
glslangValidator -D -V -S vert -e main hlsl/moments_vert.hlsl -o ../../../aether3d_build/Samples/shaders/moments_vert.spv
glslangValidator -D -V -S vert -e main hlsl/moments_skin_vert.hlsl -o ../../../aether3d_build/Samples/shaders/moments_skin_vert.spv
glslangValidator -D -V -S frag -e main hlsl/moments_frag.hlsl -o ../../../aether3d_build/Samples/shaders/moments_frag.spv
glslangValidator -D -V -S vert -e main hlsl/skybox_vert.hlsl -o ../../../aether3d_build/Samples/shaders/skybox_vert.spv
glslangValidator -D -V -S frag -e main hlsl/skybox_frag.hlsl -o ../../../aether3d_build/Samples/shaders/skybox_frag.spv
glslangValidator -D -V -S vert -e main hlsl/depthnormals_vert.hlsl -o ../../../aether3d_build/Samples/shaders/depthnormals_vert.spv
glslangValidator -D -V -S vert -e main hlsl/depthnormals_skin_vert.hlsl -o ../../../aether3d_build/Samples/shaders/depthnormals_skin_vert.spv
glslangValidator -D -V -S frag -e main hlsl/depthnormals_frag.hlsl -o ../../../aether3d_build/Samples/shaders/depthnormals_frag.spv
glslangValidator -D -V -S comp -e CSMain hlsl/LightCuller.hlsl -o ../../../aether3d_build/Samples/shaders/LightCuller.spv
glslangValidator -D -V -S vert -e main hlsl/Standard_vert.hlsl -o ../../../aether3d_build/Samples/shaders/Standard_vert.spv
glslangValidator -D -V -S vert -e main hlsl/Standard_skin_vert.hlsl -o ../../../aether3d_build/Samples/shaders/Standard_skin_vert.spv
glslangValidator -D -V -S frag -e main hlsl/Standard_frag.hlsl -o ../../../aether3d_build/Samples/shaders/Standard_frag.spv
glslangValidator -D -DENABLE_SHADOWS -V -S frag -e main hlsl/Standard_frag.hlsl -o ../../../aether3d_build/Samples/shaders/Standard_frag_shadow.spv
glslangValidator -D -DENABLE_SHADOWS_POINT -V -S frag -e main hlsl/Standard_frag.hlsl -o ../../../aether3d_build/Samples/shaders/Standard_frag_shadow_point.spv
glslangValidator -D -V -S comp -e CSMain hlsl/Bloom.hlsl -o ../../../aether3d_build/Samples/shaders/Bloom.spv
glslangValidator -D -V -S comp -e CSMain hlsl/Blur.hlsl -o ../../../aether3d_build/Samples/shaders/Blur.spv
glslangValidator -D -V -S comp -e CSMain hlsl/ssao.hlsl -o ../../../aether3d_build/Samples/shaders/ssao.spv
glslangValidator -D -V -S comp -e CSMain hlsl/compose.hlsl -o ../../../aether3d_build/Samples/shaders/compose.spv
