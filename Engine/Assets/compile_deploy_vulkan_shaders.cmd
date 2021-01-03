%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S vert -e main hlsl\sprite_vert.hlsl -o ..\..\..\aether3d_build\Samples\shaders\sprite_vert.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S frag -e main hlsl\sprite_frag.hlsl -o ..\..\..\aether3d_build\Samples\shaders\sprite_frag.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S vert -e main hlsl\unlit_cube_vert.hlsl -o ..\..\..\aether3d_build\Samples\shaders\unlit_cube_vert.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S frag -e main hlsl\unlit_cube_frag.hlsl -o ..\..\..\aether3d_build\Samples\shaders\unlit_cube_frag.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S vert -e main hlsl\unlit_vert.hlsl -o ..\..\..\aether3d_build\Samples\shaders\unlit_vert.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S frag -e main hlsl\unlit_frag.hlsl -o ..\..\..\aether3d_build\Samples\shaders\unlit_frag.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S vert -e main hlsl\unlit_skin_vert.hlsl -o ..\..\..\aether3d_build\Samples\shaders\unlit_skin_vert.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S vert -e main hlsl\moments_vert.hlsl -o ..\..\..\aether3d_build\Samples\shaders\moments_vert.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S vert -e main hlsl\moments_skin_vert.hlsl -o ..\..\..\aether3d_build\Samples\shaders\moments_skin_vert.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S frag -e main hlsl\moments_frag.hlsl -o ..\..\..\aether3d_build\Samples\shaders\moments_frag.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S vert -e main hlsl\skybox_vert.hlsl -o ..\..\..\aether3d_build\Samples\shaders\skybox_vert.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S frag -e main hlsl\skybox_frag.hlsl -o ..\..\..\aether3d_build\Samples\shaders\skybox_frag.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S vert -e main hlsl\depthnormals_vert.hlsl -o ..\..\..\aether3d_build\Samples\shaders\depthnormals_vert.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S vert -e main hlsl\depthnormals_skin_vert.hlsl -o ..\..\..\aether3d_build\Samples\shaders\depthnormals_skin_vert.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S frag -e main hlsl\depthnormals_frag.hlsl -o ..\..\..\aether3d_build\Samples\shaders\depthnormals_frag.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S comp -e CSMain hlsl\LightCuller.hlsl -o ..\..\..\aether3d_build\Samples\shaders\LightCuller.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S vert -e main hlsl\Standard_vert.hlsl -o ..\..\..\aether3d_build\Samples\shaders\Standard_vert.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S vert -e main hlsl\Standard_skin_vert.hlsl -o ..\..\..\aether3d_build\Samples\shaders\Standard_skin_vert.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S frag -e main hlsl\Standard_frag.hlsl -o ..\..\..\aether3d_build\Samples\shaders\Standard_frag.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -DENABLE_SHADOWS -V -S frag -e main hlsl\Standard_frag.hlsl -o ..\..\..\aether3d_build\Samples\shaders\Standard_frag_shadow.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S comp -e CSMain hlsl\Bloom.hlsl -o ..\..\..\aether3d_build\Samples\shaders\Bloom.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S comp -e CSMain hlsl\Blur.hlsl -o ..\..\..\aether3d_build\Samples\shaders\Blur.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S comp -e CSMain hlsl\ssao.hlsl -o ..\..\..\aether3d_build\Samples\shaders\ssao.spv
%VULKAN_SDK%\bin\glslangvalidator.exe -D -V -S comp -e CSMain hlsl\compose.hlsl -o ..\..\..\aether3d_build\Samples\shaders\compose.spv

@REM %VULKAN_SDK%\Bin\spirv-opt hlsl\sprite_vert.spv --inline-entry-points-exhaustive --convert-local-access-chains --eliminate-local-single-block --eliminate-local-single-store --eliminate-insert-extract --eliminate-dead-code-aggressive --eliminate-dead-branches --merge-blocks --eliminate-local-single-block --eliminate-local-single-store --eliminate-local-multi-store --eliminate-insert-extract --eliminate-dead-code-aggressive --eliminate-common-uniform -o ..\..\..\aether3d_build\Samples\sprite_vert_opt.spv
@REM %VULKAN_SDK%\Bin\spirv-remap --strip all --dce all -i hlsl\sprite_vert_opt.spv -o ..\..\..\aether3d_build\Samples\

pause

