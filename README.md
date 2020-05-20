# Aether3D Game Engine

[![Build Status](https://travis-ci.org/bioglaze/aether3d.svg?branch=master)](https://travis-ci.org/bioglaze/aether3d)

This codebase will evolve into the next generation [Aether3D](http://twiren.kapsi.fi/aether3d.html). More info: http://bioglaze.blogspot.fi/2014/12/planning-aether3d-rewrite-for-2015.html

Not expected to be a valuable tool before 1.0 unless for learning purposes. Current version: 0.8.1

Author: [Timo Wirén](http://twiren.kapsi.fi)

![Screenshot](/Engine/Assets/sample.jpg)
![Screenshot](/Engine/Assets/editor.jpg)

# Features

  - Windows, macOS, iOS and Linux support.
  - Vulkan, D3D12 and Metal renderers.
  - Forward+ light culling.
  - Component-based game object system.
  - VR support in Vulkan backend. Tested on HTC Vive.
  - Sprite rendering, texture atlasing and batching.
  - Bitmap and Signed Distance Field font rendering using BMFont fonts.
  - Skinned animation for meshes imported from FBX.
  - Variance shadow mapping.
  - Bloom
  - Audio support for .wav and .ogg.
  - Hot-reloading of assets.
  - Custom model format with .obj, .fbx and Blender exporter.
  - Virtual file system for .pak files.
  - Xbox controller support.
  - Cross-Platform WYSIWYG scene editor.
  - Statically linked into your application.
  - Wireframe rendering mode.
  - Line rendering.

# Planned Features

  - Physically-based rendering
  - Particles
  - Tone mapping
  - DoF
  - Most of the features in my [previous engine](http://twiren.kapsi.fi/aether3d.html)

# Sample Code

```C

    Window::Create( width, height, WindowCreateFlags::Fullscreen );
	
    RenderTexture cameraTex;
    cameraTex.Create2D( width, height, RenderTexture::DataType::Float, TextureWrap::Clamp, TextureFilter::Linear, "cameraTex" );

    GameObject camera;
    camera.AddComponent<CameraComponent>();
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0, 0, 0 ) );
    camera.GetComponent<CameraComponent>()->SetProjectionType( CameraComponent::ProjectionType::Perspective );
    camera.GetComponent<CameraComponent>()->SetProjection( 45, (float)width / (float)height, 0.1f, 200 );

    Mesh sphereMesh;
    sphereMesh.Load( FileSystem::FileContents( "sphere.ae3d" ) );

    GameObject sphere;
    sphere.AddComponent< MeshRendererComponent >();
    sphere.GetComponent< MeshRendererComponent >()->SetMesh( &sphereMesh );
    sphere.AddComponent< TransformComponent >();
    sphere.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 4, -80 } );

    Scene scene;
    scene.Add( &camera );
    scene.Add( &sphere );
    
```

# API documentation

[link](http://twiren.kapsi.fi/doc_v0.8.1/html/)

# Build

  - After building build artifacts can be found in aether3d_build next to aether3d.
  - Grab the [sample asset archive](http://twiren.kapsi.fi/files/aether3d_sample_v0.8.1.zip) and extract it into aether3d_build/Samples after building.

## Windows

  - Open the project in Engine\VisualStudio_* in Visual Studio and build it. For MinGW you can use Engine\Makefile_Vulkan.
  - Build and run Samples\01_OpenWindow. On MinGW the command is `make`
  - Vulkan users: built-in shader sources are located in aether3d_build\Samples that comes with sample asset archive. If you modify them, you can build and deploy them by running compile_deploy_vulkan_shaders.cmd in Engine/Assets. 
  - FBX converter tries to find FBX SDK 2019.5 in its default install location (English language localization)
  
### OpenVR
  - Copy OpenVR headers into Engine\ThirdParty\headers
  - Copy OpenVR DLL into aether3d_build\Samples
  - Open Vulkan renderer's Visual Studio solution and select target ***OpenVR Release***
  - Build and run Samples\04_Misc3D target ***Release Vulkan OpenVR*** in Visual Studio.

## macOS

  - Open the project Engine/Aether3D_OSX_Metal in Xcode and build it.
  - Open the project Samples/MetalSampleOSX and run it. 
  - FBX converter tries to find FBX SDK 2019.5 in its default install location.

## Linux

  - Install dependencies:

`sudo apt install libopenal-dev libx11-xcb-dev libxcb1-dev libxcb-ewmh-dev libxcb-icccm4-dev libxcb-keysyms1-dev`

  - Run `make -f Makefile_Vulkan` in Engine.

## iOS
  - Build Aether3D_iOS in Engine. It creates a framework.
  - Copy the framework into your desktop.
  - Open Samples/MetalSampleiOS and build and run it.

# Tested GPUs
  - AMD Radeon R9 Nano on Ubuntu 18.04 and Windows 10
  - NVIDIA GTX 750M on MacBook Pro 2013 and macOS High Sierra
  - NVIDIA GTX 1080 on Windows 10
  - NVIDIA RTX 2060 on Windows 10
  - Intel Skylake HD530 on Ubuntu 18.04

# Running Tests

## Visual Studio

  - Unit test project can be found in Engine\Tests\UnitTests. You'll need to set it to run in x64 and copy OpenAL32.dll into the build folder.

## GCC or Clang

  - You can find Makefiles in Engine/Tests.

# License

The engine is licensed under zlib license.

Third party library licenses are:

  - stb_image.c is in public domain
  - stb_vorbis.c is in public domain
  - OpenAL-soft is under LGPLv2 license
  - Nuklear UI is in public domain

# Roadmap/internal TODO

https://docs.google.com/document/d/1jKuEkIUHiFtF4Ys2-duCNKfV0SrxGgCSN_A2GhWeLDw/edit?usp=sharing
