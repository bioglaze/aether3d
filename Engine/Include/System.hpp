#ifndef SYSTEM_H
#define SYSTEM_H

#include <string>

#if RENDERER_METAL
#import <QuartzCore/CAMetalLayer.h>
#import <MetalKit/MetalKit.h>
#endif
#include "Vec3.hpp"

#if !defined( RENDERER_D3D12 ) && !defined( RENDERER_VULKAN ) && !defined( RENDERER_METAL ) && !defined( RENDERER_OPENGL ) && !defined( RENDERER_NULL )
#error No renderer defined
#endif

/**
   \mainpage Aether3D Game Engine v0.6 Documentation

   \section Introduction

   Aether3D is an open source game engine focusing on simplicity, robustness, portability and forward-thinking.
   <br>

   \section Features

   <ul>
   <li>Windows, macOS, iOS and GNU/Linux.</li>
   <li>OpenGL 4.1, Vulkan (WIP) Metal and D3D12 (WIP) renderers.</li>
   <li>Sprite batching.</li>
   <li>Audio support: .wav and .ogg.</li>
   <li>Bitmap fonts using <a href="http://angelcode.com/products/bmfont/">BMFont</a>. Also supports SDF rendering.</li>
   <li>Virtual file system aka .pak archive files for faster loading.</li>
   <li>Custom mesh format, converters included for .obj, .fbx and Blender.</li>
   <li>Shadow mapping from a directional or spot light (no lighting yet)</li>
   <li>Oculus Rift support.</li>
   <li>SIMD optimized math routines on desktop and iOS.</li>
   <li>Scene serialization.</li>
   <li>Scene editor built using Qt and WYSIWYG rendering.</li>
   <li>XBox controller support.</li>
   </ul>

   \section Requirements

   Only 64-bit project files/libraries are provided.
   <br />

   \section Compilation

   Grab the <a href="http://twiren.kapsi.fi/files/aether3d_sample_v0.5.5.zip">sample asset archive</a> and extract it into aether3d_build/Samples after building.

   \subsection win1 Windows/Visual Studio
   
   You'll need Visual Studio 2015 or newer. Build Engine/VisualStudio_GL45 or Engine/VisualStudio_Vulkan. This creates the library in aether3d_build.
   You can then build and run Samples/01_OpenWindow. The created executable will be placed in aether3d_build/Samples.
   You'll need OpenAL32.dll from OpenAL Soft either in your system directory or in the directory used to run samples/editor.
 
   \subsection win Windows/MinGW

   Run mingw32-make in Engine/.
   Then run mingw32-make in Samples/01_OpenWindow. The created executable will be placed in aether3d_build/Samples.

   \subsection win_oculus Windows/Oculus Rift

   Compile LibOVR, Logging and LibOVRKernel x64 release versions using VS2015. Copy those folders from Oculus SDK into Engine \ ThirdParty and 
   Open Engine/VisualStudio_GL45 and build target Oculus Release.
   Open Samples/04_Misc3D and build target Oculus Release.
 
   \subsection osx macOS/Xcode

   OpenGL: Build Engine/Aether3D_OSX. Make sure the built library is in aether3d_build. Then build and run Samples/01_OpenWindow, making
   sure the running folder is set to aether3d_build/Samples.

   Metal: Build Engine/Aether3D_OSX_Metal. Make sure the built library is in aether3d_build. Then build and run Samples/MetalSampleOSX, making
   sure the running folder is set to aether3d_build/Samples.

   \subsection osx_cmd macOS and GNU/Linux Command Line

   Run the Makefile in Engine. Then run the Makefile in Samples/01_OpenWindow. The created executable will be placed in aether3d_build/Samples.
   On GNU/Linux you need at least the following packages: libopenal-dev libx11-xcb-dev libxcb1-dev libxcb-ewmh-dev libxcb-icccm4-dev libxcb-keysyms1-dev

   \subsection Bugs

   <ul>
   <li>Metal: Shadow maps don't show.</li>
   <li>Oculus: Mirror texture is broken if SDK initialization fails.</li>
   </ul>
 
   \section Tools
 
   \subsection Editor
 
   Editor is used to create scenes by placing and moving GameObjects, adding components etc. It's built on top of Qt 5 so you'll need to
   install Qt development files. The .pro project can be opened with QtCreator or other tool. On Windows, you'll need OpenAL32.dll in aether3d\Tools\Editor\copy_to_output
 
   \subsection CombineFiles
 
   Aether3D internals almost never read raw files, all file access is abstracted by FileSystem to allow file contents to come from various sources.
   CombineFiles creates .pak files that contain contents of multiple files. You run it with command CombineFiles inputFile outputFile where
   inputFile is just a text file containing a list of file paths, each on their own line.

   \subsection SDF_Generator

   Generates a signed-distance field from a texture, useful for high-quality font rendering.
*/
namespace ae3d
{
    class Texture2D;

    namespace System
    {
        /// Inits audio system.
        void InitAudio();

        /// Inits the gamepad.
        void InitGamePad();

        /// Draws a texture into the screen. Should be called after Scene::Render().
        /// \param texture Texture
        /// \param x X screen coordinate in pixels
        /// \param y Y screen coordinate in pixels
        /// \param xSize X size in pixels
        /// \param ySize Y size in pixels
        /// \param xScreenSize X viewport size in pixels
        /// \param yScreenSize Y viewport size in pixels
        void Draw( Texture2D* texture, float x, float y, float xSize, float ySize, float xScreenSize, float yScreenSize );
        
        /// Releases all resources allocated by the engine. Call when exiting.
        void Deinit();

        /// Enables memory leak detection on DEBUG builds on Visual Studio.
        void EnableWindowsMemleakDetection();

        /// Loads built-in assets and shaders.
        void LoadBuiltinAssets();
        
        /// Loads OpenGL function pointers and sets backbuffer dimension. Doesn't create a context.
        void InitGfxDeviceForEditor( int width, int height );
        
#if RENDERER_METAL
        void InitMetal( id< MTLDevice > metalDevice, MTKView* view, int sampleCount );
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
			std::string GetStatistics();
            int GetDrawCallCount();
            int GetVertexBufferBindCount();
            int GetTextureBindCount();
            int GetShaderBindCount();
            int GetRenderTargetBindCount();
            int GetBarrierCallCount();
            int GetFenceCallCount();
            void GetGpuMemoryUsage( unsigned& outUsedMBytes, unsigned& outBudgetMBytes );
        }
    }
}
#endif
