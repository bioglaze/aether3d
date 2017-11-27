# Aether3D Game Engine

[![Build Status](https://travis-ci.org/bioglaze/aether3d.svg?branch=master)](https://travis-ci.org/bioglaze/aether3d)

This codebase will evolve into the next generation [Aether3D](http://twiren.kapsi.fi/aether3d.html). More info: http://bioglaze.blogspot.fi/2014/12/planning-aether3d-rewrite-for-2015.html

Not expected to be a valuable tool before 1.0 unless for learning purposes. Current version: 0.7.5.

Author: [Timo Wirén](http://twiren.kapsi.fi)

![Screenshot](/Engine/Assets/sample.jpg)

# Features

  - Windows, macOS, iOS and Linux support.
  - OpenGL 4.1, Vulkan (WIP), D3D12 and Metal renderers.
  - Forward+ light culling
  - Component-based game object system.
  - VR support. Tested on HTC Vive and Oculus Rift.
  - Sprite rendering, texture atlasing and batching.
  - Bitmap and Signed Distance Field font rendering using BMFont fonts.
  - Skinned animation for meshes imported from FBX.
  - Variance shadow mapping.
  - Audio support for .wav and .ogg.
  - Hot-reloading of assets.
  - Custom model format with .obj, .fbx and Blender exporter.
  - Virtual file system for .pak files.
  - XBox controller support.
  - Cross-Platform WYSIWYG scene editor.
  - Statically linked into your application.
  - Wireframe rendering mode.
  - Line rendering

# Planned Features

  - Physically-based rendering
  - Clustered Forward shading
  - ACES tone mapping
  - Post-processing effects
  - Most of the features in my [previous engine](http://twiren.kapsi.fi/aether3d.html)

# API documentation

[link](http://twiren.kapsi.fi/doc_v0.7.5/html/)

# Build

  - After building build artifacts can be found in aether3d_build next to aether3d.
  - Grab the [sample asset archive](http://twiren.kapsi.fi/files/aether3d_sample_v0.7.5.zip) and extract it into aether3d_build/Samples after building.

## Windows

  - Open the project in Engine\VisualStudio_* in VS2017 and build it. For MinGW you can use Engine/Makefile_OpenGL or Makefile_Vulkan.
  - Build and run Samples\01_OpenWindow.
  - Vulkan users: built-in shader sources are located in Engine\assets. If you modify them, you can build and deploy them by running compile_deploy_vulkan_shaders.cmd. 
  - FBX converter tries to find FBX SDK 2018.1.1 in its default install location (English language localization)
  
### OpenVR
  - Copy OpenVR headers into Engine\ThirdParty\headers
  - Open OpenGL renderer's Visual Studio solution and select target ***OpenVR Release***
  - Build and run Samples/04_Misc3D target ***OpenVR Release*** in VS2017.

## macOS

  - Open the project Engine/Aether3D_OSX or Engine/Aether3D_OSX_Metal in Xcode and build it or run the Makefile.
  - Open the project Samples/01_OpenWindow or Samples/MetalSampleOSX and run it or run the Makefile. 
  - FBX converter tries to find FBX SDK 2018.1.1 in its default install location.

## Linux

  - Install dependencies:

`sudo apt install libopenal-dev libx11-xcb-dev libxcb1-dev libxcb-ewmh-dev libxcb-icccm4-dev libxcb-keysyms1-dev`

  - Either run `make -f Makefile_OpenGL` or `make -f Makefile_Vulkan` in Engine.

## iOS
  - Build Aether3D_iOS in Engine. It creates a framework.
  - Copy the framework into your desktop.
  - Open Samples/MetalSampleiOS and build and run it.

# Tested GPUs
  - AMD Radeon R9 Nano on Ubuntu 17.10 and Windows 10
  - NVIDIA GTX 750M on MacBook Pro 2013 and macOS Sierra
  - NVIDIA GTX 980 on Windows 10
  - NVIDIA GTX 660 on Windows 7
  - Intel Skylake HD530 on Ubuntu 17.10

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
  - glxw is under zlib license
  - OpenAL-soft is under LGPLv2 license

# Roadmap/internal TODO

https://docs.google.com/document/d/1jKuEkIUHiFtF4Ys2-duCNKfV0SrxGgCSN_A2GhWeLDw/edit?usp=sharing
