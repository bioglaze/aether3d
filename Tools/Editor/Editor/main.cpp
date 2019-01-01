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
            
            if (event.type == WindowEventType::KeyDown || event.type == WindowEventType::KeyUp)
            {
                KeyCode keyCode = event.keyCode;

                switch( keyCode )
                {
                case KeyCode::Escape:
                    quit = true;
                    break;
                case KeyCode::Q:
                    moveDir.y = event.type == WindowEventType::KeyDown ? -velocity : 0;
                    break;
                case KeyCode::E:
                    moveDir.y = event.type == WindowEventType::KeyDown ? velocity : 0;
                    break;
                case KeyCode::A:
                    moveDir.x = event.type == WindowEventType::KeyDown ? -velocity : 0;
                    break;
                case KeyCode::D:
                    moveDir.x = event.type == WindowEventType::KeyDown ? velocity : 0;
                    break;
                case KeyCode::W:
                    moveDir.z = event.type == WindowEventType::KeyDown ? -velocity : 0;
                    break;
                case KeyCode::S:
                    moveDir.z = event.type == WindowEventType::KeyDown ? velocity : 0;
                    break;
                case KeyCode::Left:
                    svMoveSelection( sceneView, { -1, 0, 0 } );
                    break;
                case KeyCode::F:
                    svFocusOnSelected( sceneView );
                    break;
                case KeyCode::Right:
                    svMoveSelection( sceneView, { 1, 0, 0 } );
                    break;
                case KeyCode::Up:
                    svMoveSelection( sceneView, { 0, 1, 0 } );
                    break;
                case KeyCode::Down:
                    svMoveSelection( sceneView, { 0, -1, 0 } );
                    break;
                case KeyCode::Delete:
                    svDeleteGameObject( sceneView );
                    selectedGO = nullptr;
                    break;
                default: break;
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
                svHandleMouseMotion( sceneView, deltaX, deltaY );
            }
            else if (event.type == WindowEventType::Mouse1Down)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                inspector.HandleLeftMouseClick( x, y, 1 );
                svHandleLeftMouseDown( sceneView, x, y, width, height );
            }
            else if (event.type == WindowEventType::Mouse1Up)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                inspector.HandleLeftMouseClick( x, y, 0 );
                svHandleLeftMouseUp( sceneView );
                selectedGO = svSelectObject( sceneView, x, y, width, height );
            }
            else if (event.type == WindowEventType::Mouse2Down)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                isRightMouseDown = true;
            }
            else if (event.type == WindowEventType::MouseMiddleDown)
            {
                isMiddleMouseDown = true;
            }
            else if (event.type == WindowEventType::MouseMiddleUp)
            {
                isMiddleMouseDown = false;
                deltaX = 0;
                deltaY = 0;
                moveDir.x = 0;
                moveDir.y = 0;
            }
            else if (event.type == WindowEventType::Mouse2Up || event.type == WindowEventType::Mouse1Up)
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
            svRotateCamera( sceneView, -float( deltaX ) / 20, -float( deltaY ) / 20 );
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

            if (GetOpenFileName( &ofn ) != FALSE)
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
        case Inspector::Command::SaveScene:
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

            if (GetSaveFileName( &ofn ) != FALSE)
            {
                svSaveScene( sceneView, ofn.lpstrFile );
            }
#else
            char path[ 1024 ] = {};
            FILE* f = popen( "zenity --file-selection --save --title \"Save .scene file\"", "w" );

            if (!f)
            {
                System::Print( "Could not open file for saving.\n" );
                break;
            }
            
            fgets( path, 1024, f );
            fclose( f );
            if (strlen( path ) > 0)
            {
                path[ strlen( path ) - 1 ] = 0;
            }
            System::Print("path len: %d\n", strlen( path ));
            svSaveScene( sceneView, path );
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
