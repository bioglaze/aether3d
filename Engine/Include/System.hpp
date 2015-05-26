#ifndef SYSTEM_H
#define SYSTEM_H

#if AETHER3D_IOS
#import <UIKit/UIKit.h>
#import <QuartzCore/CAMetalLayer.h>
#endif

/**
   \mainpage Aether3D Game Engine v0.2 Documentation

   \section Introduction

   Aether3D is an open source game engine focusing on simplicity, robustness, portability and forward-thinking.
   <br>
   Current version only has 2D support, but 3D will soon follow.

   \section Features

   <ul>
   <li>Windows, OS X and GNU/Linux.</li>
   <li>OpenGL 4.1 and 4.5 renderers.</li>
   <li>Sprite batching.</li>
   <li>Audio support: .wav and .ogg.</li>
   <li>Bitmap fonts using <a href="http://angelcode.com/products/bmfont/">BMFont</a>.</li>
   <li>Virtual file system aka .pak archive files for faster loading.</li>
   </ul>

   \section Requirements

   Only 64-bit project files/libraries are provided.
   <br />
   There are two OpenGL renderers, 4.1 for OS X and 4.5 for Windows and GNU/Linux. At the moment you cannot
   build 4.1 renderer on Windows or GNU/Linux without manually modifying project/makefiles (You will be in the next release).

   \section Compilation

   \subsection Windows/Visual Studio
   
   You'll need Visual Studio 2013 or newer. Build Engine/VisualStudio_GL45. This creates the library in aether3d_build.
   You can then build and run Samples/01_OpenWindow. The created executable will be placed in aether3d_build/Samples.

   \subsection win Windows/MinGW

   Run mingw32-make in Engine/.
   Then run mingw32-make in Samples/01_OpenWindow. The created executable will be placed in aether3d_build/Samples.

   \subsection osx OS X/Xcode

   Build engine/Aether3D_OSX. Make sure the built library is in aether3d_build. Then build and run Samples/01_OpenWindow, making
   sure the running folder is set to aether3d_build/Samples.

   \subsection osx_cmd OS X and GNU/Linux Command Line

   Run the Makefile in Engine. Then run the Makefile in Samples/01_OpenWindow. The created executable will be placed in aether3d_build/Samples.

   \subsection Bugs

   <ul>
   <li>Audio clip hotloading does not work.</li>
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
#if AETHER3D_IOS
        void InitMetal( CAMetalLayer* metalLayer );
        void EndFrame();
        void BeginFrame();
#endif

        /**
          \param condition Condition that causes the assert to fire when true.
          \param message Assert message to be displayed when condition is true.
        */
        void Assert( bool condition, const char* message );

        /**
        Formats a message and prints it into stdout.
        Example: Print( "My name is %s\n", name.c_str() ); // where name is a std::string.
        Formatting examples:
        %d: integer
        %s: C-style null-terminated string
        %f: float
        */
        void Print( const char* format, ... );

        /// Reloads assets that have been changed on disk. Relatively slow operation, so avoid calling too often.
        void ReloadChangedAssets();
        
        namespace Statistics
        {
            int GetDrawCallCount();
        }
    }
}
#endif
