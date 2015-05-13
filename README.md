# Aether3D Game Engine
This codebase will evolve into the next generation [Aether3D]("Aether3D"). More info: http://bioglaze.blogspot.fi/2014/12/planning-aether3d-rewrite-for-2015.html

This is a work in progress.

# Features

  - Windows, OS X and Linux support.
  - OpenGL 4.1 and 4.5 renderer.
  - Component-based game object system.
  - Sprite rendering, texture atlasing and batching.
  - Bitmap font rendering using BMFont fonts.
  - Audio support for .wav and .ogg.

# Planned Features

  - Vulkan and D3D12 renderers.
  - Cross-Platform scene editor.
  - Most of the features in my [previous engine](http://twiren.kapsi.fi/aether3d.html)

# Build

  - After building build artifacts can be found in aether3d_build next to aether3d.

## Windows

  - Open the project in Engine\VisualStudio_GL45 and build it. For MinGW you can use Engine/Makefile.
  - Build and run Samples\01_OpenWindow.

## OS X / GNU/Linux

  - Open the project Engine/Aether3D_OSX in Xcode and build it or run the Makefile.
  - Open the project Samples/01_OpenWindow/OpenWindow.xcodeproj and run it or run the Makefile.

# License

The engine is licensed under zlib license.

Third party library licenses are:

  - stb_image.c is public domain
  - stb_vorbis.c is public domain
  - glxw is under zlib license
  - OpenAL-soft is under LGPLv2 license