C:\VulkanSDK\1.0.68.0\Bin\glslangValidator -D -V -S vert -e main ..\..\..\aether3d_build\Samples\sprite_vert.hlsl -o ..\..\..\aether3d_build\Samples\sprite_vert.spv
C:\VulkanSDK\1.0.68.0\Bin\glslangValidator -D -V -S frag -e main  ..\..\..\aether3d_build\Samples\sprite_frag.hlsl -o ..\..\..\aether3d_build\Samples\sprite_frag.spv
C:\VulkanSDK\1.0.68.0\Bin\glslangValidator -D -V -S vert -e main ..\..\..\aether3d_build\Samples\unlit_vert.hlsl -o ..\..\..\aether3d_build\Samples\unlit_vert.spv
C:\VulkanSDK\1.0.68.0\Bin\glslangValidator -D -V -S frag -e main  ..\..\..\aether3d_build\Samples\unlit_frag.hlsl -o ..\..\..\aether3d_build\Samples\unlit_frag.spv
C:\VulkanSDK\1.0.68.0\Bin\glslangValidator -D -V -S vert -e main  ..\..\..\aether3d_build\Samples\unlit_skin_vert.hlsl -o ..\..\..\aether3d_build\Samples\unlit_skin_vert.spv
C:\VulkanSDK\1.0.68.0\Bin\glslangValidator -D -V -S vert -e main ..\..\..\aether3d_build\Samples\skybox_vert.hlsl -o ..\..\..\aether3d_build\Samples\skybox_vert.spv
C:\VulkanSDK\1.0.68.0\Bin\glslangValidator -D -V -S frag -e main  ..\..\..\aether3d_build\Samples\skybox_frag.hlsl -o ..\..\..\aether3d_build\Samples\skybox_frag.spv
C:\VulkanSDK\1.0.68.0\Bin\glslangValidator -D -V -S vert -e main ..\..\..\aether3d_build\Samples\depthnormals_vert.hlsl -o ..\..\..\aether3d_build\Samples\depthnormals_vert.spv
C:\VulkanSDK\1.0.68.0\Bin\glslangValidator -D -V -S frag -e main  ..\..\..\aether3d_build\Samples\depthnormals_frag.hlsl -o ..\..\..\aether3d_build\Samples\depthnormals_frag.spv
C:\VulkanSDK\1.0.68.0\Bin\glslangValidator -D -V -S comp -e CSMain ..\..\..\aether3d_build\Samples\LightCuller.hlsl -o ..\..\..\aether3d_build\Samples\LightCuller.spv
C:\VulkanSDK\1.0.68.0\Bin\glslangValidator -D -V -S vert -e main ..\..\..\aether3d_build\Samples\Standard_vert.hlsl -o ..\..\..\aether3d_build\Samples\Standard_vert.spv
C:\VulkanSDK\1.0.68.0\Bin\glslangValidator -D -V -S frag -e main  ..\..\..\aether3d_build\Samples\Standard_frag.hlsl -o ..\..\..\aether3d_build\Samples\Standard_frag.spv
C:\VulkanSDK\1.0.68.0\Bin\glslangValidator -D -V -S vert -e main  ..\..\..\aether3d_build\Samples\fullscreen_triangle_vert.hlsl -o ..\..\..\aether3d_build\Samples\fullscreen_triangle_vert.spv

REM C:\VulkanSDK\1.0.68.0\Bin\spirv-opt ..\..\..\aether3d_build\Samples\sprite_vert.spv --inline-entry-points-exhaustive --convert-local-access-chains --eliminate-local-single-block --eliminate-local-single-store --eliminate-insert-extract --eliminate-dead-code-aggressive --eliminate-dead-branches --merge-blocks --eliminate-local-single-block --eliminate-local-single-store --eliminate-local-multi-store --eliminate-insert-extract --eliminate-dead-code-aggressive --eliminate-common-uniform -o ..\..\..\aether3d_build\Samples\sprite_vert_opt.spv
REM C:\VulkanSDK\1.0.68.0\Bin\spirv-remap --strip all --dce all -i ..\..\..\aether3d_build\Samples\sprite_vert_opt.spv -o ..\..\..\aether3d_build\Samples\

pause

