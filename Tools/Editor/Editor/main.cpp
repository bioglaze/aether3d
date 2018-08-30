#include <stdio.h>
#include <string.h>
#if _MSC_VER
#include <Windows.h>
#endif
#include "Array.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "Inspector.hpp"
#include "SceneView.hpp"
#include "System.hpp"
#include "Window.hpp"
#include "Vec3.hpp"

using namespace ae3d;

int main()
{
    int width = 640 * 2;
    int height = 480 * 2;
    
    System::EnableWindowsMemleakDetection();
    Window::Create( width, height, WindowCreateFlags::Empty );
    Window::SetTitle( "Aether3D Editor" );
    Window::GetSize( width, height );
    System::LoadBuiltinAssets();
    
    bool quit = false;   
    int x = 0, y = 0;

    SceneView* sceneView;
    svInit( &sceneView, width, height );

    Inspector inspector;
    inspector.Init();
    
    Vec3 moveDir;
    constexpr float velocity = 0.2f;

    int lastMouseX = 0;
    int lastMouseY = 0;
    int deltaX = 0;
    int deltaY = 0;
    bool isRightMouseDown = false;
    bool isMiddleMouseDown = false;
    
    ae3d::GameObject* selectedGO = nullptr;

    while (Window::IsOpen() && !quit)
    {
        Window::PumpEvents();
        WindowEvent event;

        inspector.BeginInput();

        while (Window::PollEvent( event ))
        {
            if (event.type == WindowEventType::Close)
            {
                quit = true;
				System::Deinit();
				return 0;
            }
            
            if (event.type == WindowEventType::KeyDown ||
                event.type == WindowEventType::KeyUp)
            {
                KeyCode keyCode = event.keyCode;

                if (keyCode == KeyCode::Escape)
                {
                    quit = true;
                }
                else if (keyCode == KeyCode::A)
                {
                    moveDir.x = event.type == WindowEventType::KeyDown ? -velocity : 0;
                }
                else if (keyCode == KeyCode::D)
                {
                    moveDir.x = event.type == WindowEventType::KeyDown ? velocity : 0;
                }
                else if (keyCode == KeyCode::W)
                {
                    moveDir.z = event.type == WindowEventType::KeyDown ? -velocity : 0;
                }
                else if (keyCode == KeyCode::S)
                {
                    moveDir.z = event.type == WindowEventType::KeyDown ? velocity : 0;
                }
                else if (keyCode == KeyCode::Left)
                {
                    svMoveSelection( sceneView, { -1, 0, 0 } );
                }
                else if (keyCode == KeyCode::Right)
                {
                    svMoveSelection( sceneView, { 1, 0, 0 } );
                }
                else if (keyCode == KeyCode::Up)
                {
                    svMoveSelection( sceneView, { 0, 1, 0 } );
                }
                else if (keyCode == KeyCode::Down)
                {
                    svMoveSelection( sceneView, { 0, -1, 0 } );
                }
            }

            if (event.type == WindowEventType::MouseMove)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                deltaX = x - lastMouseX;
                deltaY = y - lastMouseY;
                lastMouseX = x;
                lastMouseY = y;
                inspector.HandleMouseMotion( x, y );
            }

            if (event.type == WindowEventType::Mouse1Down)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                inspector.HandleLeftMouseClick( x, y, 1 );
            }

            if (event.type == WindowEventType::Mouse1Up)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                inspector.HandleLeftMouseClick( x, y, 0 );

                selectedGO = svSelectObject( sceneView, x, y, width, height );
            }

            if (event.type == WindowEventType::Mouse2Down)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                isRightMouseDown = true;
            }

            if (event.type == WindowEventType::MouseMiddleDown)
            {
                isMiddleMouseDown = true;
            }
            
            if (event.type == WindowEventType::MouseMiddleUp)
            {
                isMiddleMouseDown = false;
                deltaX = 0;
                deltaY = 0;
                moveDir.x = 0;
                moveDir.y = 0;
            }
            
            if (event.type == WindowEventType::Mouse2Up || event.type == WindowEventType::Mouse1Up)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                isRightMouseDown = false;
                deltaX = 0;
                deltaY = 0;
            }
        }

        inspector.EndInput();

        if (isRightMouseDown)
        {
#ifdef __linux__
            svRotateCamera( sceneView, float( deltaX ) / 20, float( deltaY ) / 20 );
#else
            svRotateCamera( sceneView , -float( deltaX ) / 20, -float( deltaY ) / 20 );
#endif
        }

        if (isMiddleMouseDown)
        {
            moveDir.x = deltaX / 20.0f;
            moveDir.y = -deltaY / 20.0f;
        }
        
        Inspector::Command inspectorCommand;

        svMoveCamera( sceneView, moveDir );
        svBeginRender( sceneView );
        int goCount = 0;
        GameObject** gameObjects = svGetGameObjects( sceneView, goCount );
        inspector.Render( width, height, selectedGO, inspectorCommand, gameObjects, goCount );
        svEndRender( sceneView );

        switch (inspectorCommand)
        {
        case Inspector::Command::CreateGO:
            svAddGameObject( sceneView );
            break;
        case Inspector::Command::OpenScene:
        {
#if _MSC_VER
            OPENFILENAME ofn = {};
            TCHAR szFile[ 260 ] = {};

            ofn.lStructSize = sizeof( ofn );
            ofn.hwndOwner = GetActiveWindow();
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof( szFile );
            ofn.lpstrFilter = "All\0*.*\0Scene\0*.SCENE\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = nullptr;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = nullptr;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName( &ofn ) == TRUE)
            {
                auto contents = FileSystem::FileContents( ofn.lpstrFile );
                svLoadScene( sceneView, contents );
            }
#else
            char path[ 1024 ] = {};
            FILE* f = popen( "zenity --file-selection --title \"Load .scene file\"", "r" );
            fgets( path, 1024, f );

            if (strlen( path ) > 0)
            {
                path[ strlen( path ) - 1 ] = 0;
            }
            auto contents = FileSystem::FileContents( path );
            svLoadScene( sceneView, contents );
#endif
        }
        break;
        default:
            break;
        }
        
        Window::SwapBuffers();
    }

    inspector.Deinit();
    System::Deinit();

    return 0;
}
