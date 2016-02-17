#ifndef SYSTEM_H
#define SYSTEM_H

#if AETHER3D_METAL
#import <QuartzCore/CAMetalLayer.h>
#import <MetalKit/MetalKit.h>
#endif

/**
   \mainpage Aether3D Game Engine v0.5 Documentation

   \section Introduction

   Aether3D is an open source game engine focusing on simplicity, robustness, portability and forward-thinking.
   <br>

   \section Features

   <ul>
   <li>Windows, OS X, iOS and GNU/Linux.</li>
   <li>OpenGL 4.1, Vulkan (WIP) Metal and D3D12 (WIP) renderers.</li>
   <li>Sprite batching.</li>
   <li>Audio support: .wav and .ogg.</li>
   <li>Bitmap fonts using <a href="http://angelcode.com/products/bmfont/">BMFont</a>. Also supports SDF rendering.</li>
   <li>Virtual file system aka .pak archive files for faster loading.</li>
   <li>Custom mesh format, converters included for .obj and Blender.</li>
   <li>Shadow mapping from a directional light (no lighting yet)</li>
   <li>Oculus Rift support.</li>
   <li>SIMD optimized math routines on desktop and iOS.</li>
   <li>Scene serialization.</li>
   <li>Scene editor built using Qt.</li>
   <li>XBox controller support.</li>
   </ul>

   \section Requirements

   Only 64-bit project files/libraries are provided.
   <br />

   \section Compilation

   Grab the <a href="http://twiren.kapsi.fi/files/aether3d_sample_v0.4.zip">sample asset archive</a> and extract it into aether3d_build/Samples after building.

   \subsection win1 Windows/Visual Studio
   
   You'll need Visual Studio 2015 or newer. Build Engine/VisualStudio_GL45 or Engine/VisualStudio_Vulkan. This creates the library in aether3d_build.
   You can then build and run Samples/01_OpenWindow. The created executable will be placed in aether3d_build/Samples.
   You'll need OpenAL32.dll from OpenAL Soft either in your system directory or in the directory used to run samples/editor.
 
   \subsection win Windows/MinGW

   Run mingw32-make in Engine/.
   Then run mingw32-make in Samples/01_OpenWindow. The created executable will be placed in aether3d_build/Samples.

   \subsection win_oculus Windows/Oculus Rift

   Copy LibOVR and LibOVRKernel folders from Oculus SDK into Engine \ ThirdParty and compile their release versions using VS2015.
   Open Engine/VisualStudio_GL45 and build target Release.
   Open Samples/04_Misc3D and build target Oculus Release.
 
   \subsection osx OS X/Xcode

   OpenGL: Build Engine/Aether3D_OSX. Make sure the built library is in aether3d_build. Then build and run Samples/01_OpenWindow, making
   sure the running folder is set to aether3d_build/Samples.

   Metal: Build Engine/Aether3D_OSX_Metal. Make sure the built library is in aether3d_build. Then build and run Samples/MetalSampleOSX, making
   sure the running folder is set to aether3d_build/Samples.

   \subsection osx_cmd OS X and GNU/Linux Command Line

   Run the Makefile in Engine. Then run the Makefile in Samples/01_OpenWindow. The created executable will be placed in aether3d_build/Samples.
   On GNU/Linux you need at least the following packages: libopenal-dev libxcb1-dev libxcb-ewmh-dev libxcb-icccm4-dev libxcb-keysyms1-dev

   \subsection Bugs

   <ul>
   <li>D3D12 renderer is in a very early state.</li>
   </ul>
*/
namespace ae3d
{
    namespace System
    {
        /// Inits audio system.
        void InitAudio();

        /// Inits the gamepad.
        void InitGamePad();

        /// Releases all resources allocated by the engine. Call when exiting.
        void Deinit();

        /// Enables memory leak detection on DEBUG builds on Visual Studio.
        void EnableWindowsMemleakDetection();

        /// Loads built-in assets and shaders.
        void LoadBuiltinAssets();
        
        /// Loads OpenGL function pointers and sets backbuffer dimension. Doesn't create a context.
        void InitGfxDeviceForEditor( int width, int height );
        
#if AETHER3D_METAL
        void InitMetal( id< MTLDevice > metalDevice, MTKView* view );
        void SetCurrentDrawableMetal( id <CAMetalDrawable> drawable, MTLRenderPassDescriptor* renderPass );
        void BeginFrame();
#endif

        /**
          \param condition Condition that causes the assert to fire when true.
          \param message Assert message to be displayed when condition is true.
        */
        void Assert( bool condition, const char* message );

        /**
        Formats a message and prints it into stdout.
        Example: Print( "My name is %s", name.c_str() ); // where name is a std::string.
        Formatting examples:
        %d: integer
        %s: C-style null-terminated string
        %f: float
        %x: hexadecimal
        */
        void Print( const char* format, ... );

        /// Reloads assets that have been changed on disk. Relatively slow operation, so avoid calling too often.
        void ReloadChangedAssets();
        
        namespace Statistics
        {
            int GetDrawCallCount();
            int GetVertexBufferBindCount();
            int GetTextureBindCount();
            int GetShaderBindCount();
            int GetRenderTargetBindCount();
        }
    }
}
#endif
