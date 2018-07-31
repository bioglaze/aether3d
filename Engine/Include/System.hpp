#ifndef SYSTEM_H
#define SYSTEM_H

#include <string>

#if RENDERER_METAL
#import <QuartzCore/CAMetalLayer.h>
#import <MetalKit/MetalKit.h>
#endif

#if !defined( RENDERER_D3D12 ) && !defined( RENDERER_VULKAN ) && !defined( RENDERER_METAL )
#error No renderer defined
#endif

/**
   \mainpage Aether3D Game Engine v0.7.8 Documentation

   \section Introduction

   Aether3D is an open source game engine focusing on simplicity, robustness, portability and forward-thinking.
   <br>

   \section Features

   <ul>
   <li>Windows, macOS, iOS and GNU/Linux.</li>
   <li>Vulkan (WIP) Metal and D3D12 renderers.</li>
   <li>Sprite batching.</li>
   <li>Audio support: .wav and .ogg.</li>
   <li>Bitmap fonts using <a href="http://angelcode.com/products/bmfont/">BMFont</a>. Also supports SDF rendering.</li>
   <li>Virtual file system aka .pak archive files for faster loading.</li>
   <li>Custom mesh format, converters included for .obj, .fbx and Blender.</li>
   <li>Shadow mapping from a directional or spot light (no lighting yet)</li>
   <li>VR support. Tested on HTC Vive.</li>
   <li>SIMD optimized math routines on desktop and iOS.</li>
   <li>Skinned animation.</li>
   <li>Scene serialization.</li>
   <li>Scene editor built using WYSIWYG rendering.</li>
   <li>XBox controller support.</li>
   <li>Wireframe rendering.</li>
   <li>Line rendering.</li>
   </ul>

   \section Requirements

   Only 64-bit project files/libraries are provided.
   <br />

   \section Compilation

   Grab the <a href="http://twiren.kapsi.fi/files/aether3d_sample_v0.7.8.zip">sample asset archive</a> and extract it into aether3d_build/Samples after building.

   \subsection win1 Windows/Visual Studio
   
   You'll need Visual Studio 2017 or newer. Build Engine/VisualStudio_D3D12 or Engine/VisualStudio_Vulkan. This creates the library in aether3d_build.
   You can then build and run Samples/01_OpenWindow. The created executable will be placed in aether3d_build/Samples.
   You'll need OpenAL32.dll from OpenAL Soft either in your system directory or in the directory used to run samples/editor.
 
   \subsection win Windows/MinGW

   Run mingw32-make in Engine/.
   Then run mingw32-make -f Makefile_Vulkan in Samples/01_OpenWindow. The created executable will be placed in aether3d_build/Samples.

   \subsection osx macOS/Xcode

   Metal: Build Engine/Aether3D_OSX_Metal. Make sure the built library is in aether3d_build. Then build and run Samples/MetalSampleOSX, making
   sure the running folder is set to aether3d_build/Samples.

   \subsection osx_cmd macOS and GNU/Linux Command Line

   Run Makefile_Vulkan (on Linux) in Engine. Then run the Makefile in Samples/01_OpenWindow. The created executable will be placed in aether3d_build/Samples.
   On GNU/Linux you need at least the following packages: <pre>libopenal-dev libx11-xcb-dev libxcb1-dev libxcb-ewmh-dev libxcb-icccm4-dev libxcb-keysyms1-dev</pre>

   \section Tools
 
   \subsection Editor
 
   Editor is used to create scenes by placing and moving GameObjects, adding components etc.
 
   \subsection CombineFiles
 
   Aether3D internals almost never read raw files, all file access is abstracted by FileSystem to allow file contents to come from various sources.
   CombineFiles creates .pak files that contain contents of multiple files. You run it with command <code>CombineFiles inputFile outputFile</code> where
   inputFile is just a text file containing a list of file paths, each on their own line.

   \subsection SDF_Generator

   Generates a signed-distance field from a texture, useful for high-quality font rendering.
*/
namespace ae3d
{
    struct Matrix44;
	struct Vec3;
	struct Vec4;
    class Texture2D;
    class TextureBase;
    
    /// High-level functions
    namespace System
    {
        /// \param vertexSize Vertex size.
        /// \param indexSize Index size.
        /// \param outMappedVertices Mapped vertices.
        /// \param outMappedIndices Mapped indices.
        void MapUIVertexBuffer( int vertexSize, int indexSize, void** outMappedVertices, void** outMappedIndices );

        /// Unmaps UI vertex buffer. Must be called before DrawUI if it was mapped.
        void UnmapUIVertexBuffer();
        
        /// Inits audio system.
        void InitAudio();

        /// Inits the gamepad.
        void InitGamePad();

        /// Creates a buffer for line drawing.
        /// \param lines Lines.
        /// \param color Color.
        /// \return handle to the buffer. Used with DrawLines.
        int CreateLineBuffer( const Vec3* lines, int lineCount, const Vec3& color );
        
        /// Draws a texture into the screen. Should be called after Scene::Render().
        /// \param texture Texture
        /// \param x X screen coordinate in pixels
        /// \param y Y screen coordinate in pixels
        /// \param xSize X size in pixels
        /// \param ySize Y size in pixels
        /// \param xScreenSize X viewport size in pixels
        /// \param yScreenSize Y viewport size in pixels
        /// \param tintColor Tint color
        /// \param blend Enable blending
        void Draw( TextureBase* texture, float x, float y, float xSize, float ySize, float xScreenSize, float yScreenSize, const Vec4& tintColor, bool blend );
        
        /// Draws lines using a line loop. Should be called after Scene::Render().
        /// \param handle Handle, created with CreateLineBuffer.
        /// \param view Camera's view matrix.
        /// \param projection Camera's projection matrix.
        void DrawLines( int handle, const Matrix44& view, const Matrix44& projection );
        
        /// \param vpX Viewport x.
        /// \param vpY Viewport y.
        /// \param vpWidth Viewport width.
        /// \param vpHeight Viewport height.
        /// \param elemCount Element count.
        /// \param texture Texture
        /// \param offset Offset into vertex buffer.
        /// \param windowWidth Window width in pixels.
        /// \param windowHeight Window height in pixels.
        void DrawUI( int vpX, int vpY, int vpWidth, int vpHeight, int elemCount, Texture2D* texture, void* offset, int windowWidth, int windowHeight );

        /// Releases all resources allocated by the engine. Call when exiting.
        void Deinit();

        /// Enables memory leak detection on DEBUG builds on Visual Studio.
        void EnableWindowsMemleakDetection();

        /// Loads built-in assets and shaders.
        void LoadBuiltinAssets();
        
#if RENDERER_METAL
        void InitMetal( id< MTLDevice > metalDevice, MTKView* view, int sampleCount, int uiVBSize, int uiIBSize );
        void SetCurrentDrawableMetal( MTKView* view );
        void BeginFrame();
        void EndFrame();
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

        /// Tests internal functionality.
        void RunUnitTests();

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
