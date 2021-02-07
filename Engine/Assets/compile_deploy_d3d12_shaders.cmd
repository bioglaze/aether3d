SET COMPILER="C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64\fxc"
REM SET COMPILER=dxc

%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T ps_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\depthnormals_frag.obj hlsl\depthnormals_frag.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T vs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\depthnormals_vert.obj hlsl\depthnormals_vert.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T vs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\depthnormals_skin_vert.obj hlsl\depthnormals_skin_vert.hlsl

%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T ps_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\moments_frag.obj hlsl\moments_frag.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T vs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\moments_vert.obj hlsl\moments_vert.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T vs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\moments_skin_vert.obj hlsl\moments_skin_vert.hlsl

%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T ps_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\sdf_frag.obj hlsl\sdf_frag.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T vs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\sdf_vert.obj hlsl\sdf_vert.hlsl

%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T ps_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\skybox_frag.obj hlsl\skybox_frag.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T vs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\skybox_vert.obj hlsl\skybox_vert.hlsl

%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T ps_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\sprite_frag.obj hlsl\sprite_frag.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T vs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\sprite_vert.obj hlsl\sprite_vert.hlsl

%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T ps_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\Standard_frag.obj hlsl\Standard_frag.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T ps_5_1 /DENABLE_SHADOWS /Fo ..\..\..\aether3d_build\Samples\shaders\Standard_frag_shadow.obj hlsl\Standard_frag.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T ps_5_1 /DENABLE_SHADOWS_POINT /Fo ..\..\..\aether3d_build\Samples\shaders\Standard_frag_shadow_point.obj hlsl\Standard_frag.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T vs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\Standard_vert.obj hlsl\Standard_vert.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T vs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\Standard_skin_vert.obj hlsl\Standard_skin_vert.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T ps_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\unlit_cube_frag.obj hlsl\unlit_cube_frag.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T vs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\unlit_cube_vert.obj hlsl\unlit_cube_vert.hlsl

%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T ps_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\unlit_frag.obj hlsl\unlit_frag.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T vs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\unlit_vert.obj hlsl\unlit_vert.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T vs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\unlit_skin_vert.obj hlsl\unlit_skin_vert.hlsl

%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T cs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\LightCuller.obj /E CSMain hlsl\LightCuller.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T cs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\Bloom.obj /E CSMain hlsl\Bloom.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T cs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\Blur.obj /E CSMain hlsl\Blur.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T cs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\ssao.obj /E CSMain hlsl\ssao.hlsl
%COMPILER% /nologo /all_resources_bound /Ges /WX /O3 /Zi /T cs_5_1 /Fo ..\..\..\aether3d_build\Samples\shaders\compose.obj /E CSMain hlsl\compose.hlsl

pause
