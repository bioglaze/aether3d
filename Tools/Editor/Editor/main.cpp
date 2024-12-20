// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
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

#if _MSC_VER
void GetOpenPath( char* path, const char* extension )
{
    OPENFILENAME ofn = {};
    TCHAR szFile[ 260 ] = {};

    ofn.lStructSize = sizeof( ofn );
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof( szFile );
    if (strstr( extension, "ae3d" ))
    {
        ofn.lpstrFilter = "Mesh\0*.ae3d\0All\0*.*\0";
    }
    else if (strstr( extension, "scene" ))
    {
        ofn.lpstrFilter = "Scene\0*.scene\0All\0*.*\0";
    }
    else if (strstr( extension, "wav" ))
    {
        ofn.lpstrFilter = "Audio\0*.wav;*.ogg\0All\0*.*\0";
    }

    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName( &ofn ) != FALSE)
    {
        strncpy( path, ofn.lpstrFile, 1024 );
    }
}
#else
void GetOpenPath( char* path, const char* extension )
{
    FILE* f = nullptr;

    if (strstr( extension, "scene" ))
    {
        f = popen( "zenity --file-selection --file-filter=*.scene --title \"Load .scene or .ae3d file\"", "r" );
    }
    else if (strstr( extension , "ae3d" ))
    {
        f = popen( "zenity --file-selection --file-filter=*.ae3d --title \"Load .scene or .ae3d file\"", "r" );
    }
    else if (strstr( extension, "wav" ))
    {
        f = popen( "zenity --file-selection --file-filter=*.wav --title \"Load .scene or .ae3d file\"", "r" );
    }
    else
    {
        f = popen( "zenity --file-selection --title \"Load .scene or .ae3d file\"", "r" );
    }
    
    fgets( path, 1024, f );
    
    if (strlen( path ) > 0)
    {
        path[ strlen( path ) - 1 ] = 0;
    }
}
#endif

int main()
{
    int width = 1920 / 1;
    int height = 1080 / 1;
    
    System::EnableWindowsMemleakDetection();
    Window::Create( width, height, WindowCreateFlags::Empty );
    Window::SetTitle( "Aether3D Editor" );
    Window::GetSize( width, height );
    System::LoadBuiltinAssets();
    System::InitGamePad();
    System::InitAudio();

    bool quit = false;   
    int x = 0, y = 0;

    SceneView* sceneView;
    svInit( &sceneView, width, height );

    Inspector inspector;
    inspector.Init();
    
    Vec3 moveDir;
    Vec3 gamepadMoveDir;
    constexpr float velocity = 0.2f;
    constexpr int inspectorWidth = 450;    

    int lastMouseX = 0;
    int lastMouseY = 0;
    int deltaX = 0;
    int deltaY = 0;
    int startMouseX = 0;
    int startMouseY = 0;
    
    bool isRightMouseDown = false;
    bool isMiddleMouseDown = false;

    ae3d::GameObject* selectedGO = nullptr;

    bool mouseMoveHandled = false;

    while (Window::IsOpen() && !quit)
    {
        mouseMoveHandled = false;
        
        Window::PumpEvents();
        WindowEvent event;

        svUpdate( sceneView );
        
        inspector.BeginInput();

        deltaX = 0;
        deltaY = 0;
        
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
                if (!inspector.IsTextEditActive())
                {

                    KeyCode keyCode = event.keyCode;

                    switch (keyCode)
                    {
                    case KeyCode::Q:
                        moveDir.y = event.type == WindowEventType::KeyDown ? -velocity : 0;
                        break;
                    case KeyCode::E:
                        moveDir.y = event.type == WindowEventType::KeyDown ? velocity : 0;
                        break;
                    case KeyCode::A:
                        moveDir.x = event.type == WindowEventType::KeyDown ? -velocity : 0;
                        if (event.keyModifier == KeyModifier::Shift)
                        {
                            moveDir.x *= 2;
                        }

                        break;
                    case KeyCode::D:
                        if (event.keyModifier == KeyModifier::Control)
                        {
                            svDuplicateGameObject( sceneView );
                        }
                        else
                        {
                            moveDir.x = event.type == WindowEventType::KeyDown ? velocity : 0;
                            if (event.keyModifier == KeyModifier::Shift)
                            {
                                moveDir.x *= 2;
                            }
                        }
                        break;
                    case KeyCode::W:
                        moveDir.z = event.type == WindowEventType::KeyDown ? -velocity : 0;
                        if (event.keyModifier == KeyModifier::Shift)
                        {
                            moveDir.z *= 2;
                        }
                        break;
                    case KeyCode::S:
                        moveDir.z = event.type == WindowEventType::KeyDown ? velocity : 0;
                        if (event.keyModifier == KeyModifier::Shift)
                        {
                            moveDir.z *= 2;
                        }
                        break;
                    case KeyCode::F:
                        svFocusOnSelected( sceneView );
                        break;
                    case KeyCode::Left:
                        {
                            float sign = svGetCameraSign( sceneView );
                            float sign2 = sign > 0 ? 1 : -1;
                            svMoveSelection( sceneView, { -1 * sign2, 0, 0 } );
                        }
                        break;
                    case KeyCode::Right:
                        {
                            float sign = svGetCameraSign( sceneView );
                            float sign2 = sign > 0 ? 1 : -1;
                            svMoveSelection( sceneView, { 1 * sign2, 0, 0 } );
                        }
                        break;
                    case KeyCode::Up:
                        svMoveSelection( sceneView, { 0, 1, 0 } );
                        break;
                    case KeyCode::Down:
                        svMoveSelection( sceneView, { 0, -1, 0 } );
                        break;
                    case KeyCode::PageUp:
                        svMoveSelection( sceneView, { 0, 0, 1 } );
                        break;
                    case KeyCode::PageDown:
                        svMoveSelection( sceneView, { 0, 0, -1 } );
                        break;
                    case KeyCode::Delete:
                        svDeleteGameObject( sceneView );
                        selectedGO = nullptr;
                        break;
                    case KeyCode::Escape:
                        selectedGO = svSelectObject( sceneView, -1, -1, width, height );
                        break;
                    default: break;
                    }
                }

                if (event.keyCode == KeyCode::Enter) inspector.HandleKey( 4, event.type == WindowEventType::KeyDown ? 0 : 1 );
                if (event.keyCode == KeyCode::Backspace) inspector.HandleKey( 6, event.type == WindowEventType::KeyDown ? 0 : 1 );
                if (event.keyCode == KeyCode::Delete) inspector.HandleKey( 3, event.type == WindowEventType::KeyDown ? 0 : 1 );
                if (event.keyCode == KeyCode::Home) inspector.HandleKey( 17, event.type == WindowEventType::KeyDown ? 0 : 1 );
                if (event.keyCode == KeyCode::End) inspector.HandleKey( 18, event.type == WindowEventType::KeyDown ? 0 : 1 );
                if (event.type == WindowEventType::KeyDown)
                {
                    if (event.keyCode == KeyCode::Minus) inspector.HandleChar( '-' );
                    if (event.keyCode == KeyCode::Plus) inspector.HandleChar( '+' );
                    if (event.keyCode == KeyCode::Dot) inspector.HandleChar( '.' );
                    if (event.keyCode == KeyCode::N0) inspector.HandleChar( 48 );
                    if (event.keyCode == KeyCode::N1) inspector.HandleChar( 49 );
                    if (event.keyCode == KeyCode::N2) inspector.HandleChar( 50 );
                    if (event.keyCode == KeyCode::N3) inspector.HandleChar( 51 );
                    if (event.keyCode == KeyCode::N4) inspector.HandleChar( 52 );
                    if (event.keyCode == KeyCode::N5) inspector.HandleChar( 53 );
                    if (event.keyCode == KeyCode::N6) inspector.HandleChar( 54 );
                    if (event.keyCode == KeyCode::N7) inspector.HandleChar( 55 );
                    if (event.keyCode == KeyCode::N8) inspector.HandleChar( 56 );
                    if (event.keyCode == KeyCode::N9) inspector.HandleChar( 57 );

                    if (inspector.IsTextEditActive())
                    {
                        if (event.keyCode == KeyCode::Left) inspector.HandleKey( 12, 1 ); // NK_KEY_LEFT
                        if (event.keyCode == KeyCode::Right) inspector.HandleKey( 13, 1 ); // NK_KEY_RIGHT
                        if (event.keyCode == KeyCode::Q) inspector.HandleChar( 'q' );
                        if (event.keyCode == KeyCode::W) inspector.HandleChar( 'w' );
                        if (event.keyCode == KeyCode::E) inspector.HandleChar( 'e' );
                        if (event.keyCode == KeyCode::R) inspector.HandleChar( 'r' );
                        if (event.keyCode == KeyCode::T) inspector.HandleChar( 't' );
                        if (event.keyCode == KeyCode::Y) inspector.HandleChar( 'y' );
                        if (event.keyCode == KeyCode::U) inspector.HandleChar( 'u' );
                        if (event.keyCode == KeyCode::I) inspector.HandleChar( 'i' );
                        if (event.keyCode == KeyCode::O) inspector.HandleChar( 'o' );
                        if (event.keyCode == KeyCode::P) inspector.HandleChar( 'p' );
                        if (event.keyCode == KeyCode::A) inspector.HandleChar( 'a' );
                        if (event.keyCode == KeyCode::S) inspector.HandleChar( 's' );
                        if (event.keyCode == KeyCode::D) inspector.HandleChar( 'd' );
                        if (event.keyCode == KeyCode::F) inspector.HandleChar( 'f' );
                        if (event.keyCode == KeyCode::G) inspector.HandleChar( 'g' );
                        if (event.keyCode == KeyCode::H) inspector.HandleChar( 'h' );
                        if (event.keyCode == KeyCode::J) inspector.HandleChar( 'j' );
                        if (event.keyCode == KeyCode::K) inspector.HandleChar( 'k' );
                        if (event.keyCode == KeyCode::L) inspector.HandleChar( 'l' );
                        if (event.keyCode == KeyCode::Z) inspector.HandleChar( 'z' );
                        if (event.keyCode == KeyCode::X) inspector.HandleChar( 'x' );
                        if (event.keyCode == KeyCode::C) inspector.HandleChar( 'c' );
                        if (event.keyCode == KeyCode::V) inspector.HandleChar( 'v' );
                        if (event.keyCode == KeyCode::B) inspector.HandleChar( 'b' );
                        if (event.keyCode == KeyCode::N) inspector.HandleChar( 'n' );
                        if (event.keyCode == KeyCode::M) inspector.HandleChar( 'm' );
                        if (event.keyCode == KeyCode::Space) inspector.HandleChar( ' ' );
                    }
                }
            }

            if (event.type == WindowEventType::MouseMove && !mouseMoveHandled)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                deltaX = x - lastMouseX;
                deltaY = y - lastMouseY;
                lastMouseX = x;
                lastMouseY = y;
                mouseMoveHandled = true;
                inspector.HandleMouseMotion( x, y );
                const bool snapToGrid = event.keyModifier == ae3d::KeyModifier::Control;
                svHandleMouseMotion( sceneView, deltaX, deltaY );
            }
            else if (event.type == WindowEventType::Mouse1Down)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                startMouseX = x;
                startMouseY = y;
                
                const bool clickedOnInspector = x < inspectorWidth;

                if (clickedOnInspector)
                {
                    inspector.HandleLeftMouseClick( x, y, 1 );
                }
                else
                {
                    svHandleLeftMouseDown( sceneView, x, y, width, height );
                    
                    selectedGO = nullptr;
                }
            }
            else if (event.type == WindowEventType::Mouse1Up)
            {
                x = event.mouseX;
                y = height - event.mouseY;

                startMouseX = 0;
                startMouseY = 0;
                
                svHandleLeftMouseUp( sceneView );

                const bool clickedOnInspector = x < 450;

                if (clickedOnInspector)
                {
                    inspector.HandleLeftMouseClick( x, y, 0 );
                }
                else
                {
                    if (!svIsDraggingGizmo( sceneView ))
                    {
                        selectedGO = svSelectObject( sceneView, x, y, width, height );

                        if (selectedGO)
                        {
                            inspector.SetTextEditText( selectedGO->GetName() );
                        }
                    }

                    svHandleLeftMouseUp( sceneView );
                }
            }
            else if (event.type == WindowEventType::Mouse2Up)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                isRightMouseDown = false;
                deltaX = 0;
                deltaY = 0;
            }
            else if (event.type == WindowEventType::Mouse2Down)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                const bool clickedOnInspector = x < inspectorWidth;

                isRightMouseDown = !clickedOnInspector;
            }
            else if (event.type == WindowEventType::MouseMiddleDown)
            {
                const bool clickedOnInspector = x < inspectorWidth;

                isMiddleMouseDown = !clickedOnInspector;
            }
            else if (event.type == WindowEventType::MouseMiddleUp)
            {
                isMiddleMouseDown = false;
                deltaX = 0;
                deltaY = 0;
                moveDir.x = 0;
                moveDir.y = 0;
            }
            else if (event.type == WindowEventType::MouseWheelScrollDown || event.type == WindowEventType::MouseWheelScrollUp)
            {
                inspector.HandleMouseScroll( event.mouseY );
                moveDir += Vec3( 0, 0, event.mouseWheel );
            }
            else if (event.type == WindowEventType::GamePadLeftThumbState)
            {
                gamepadMoveDir.z = -event.gamePadThumbY;
                gamepadMoveDir.x = event.gamePadThumbX;
#ifndef _MSC_VER
                moveDir = {};
#endif
            }
            else if (event.type == WindowEventType::GamePadRightThumbState)
            {
                svRotateCamera( sceneView, -float( event.gamePadThumbX ) / 10, float( event.gamePadThumbY ) / 10 );
            }
        }

        inspector.EndInput();
        svHighlightGizmo( sceneView, x, y, width, height );

        if (isRightMouseDown && event.type == WindowEventType::MouseMove)
        {
            svRotateCamera( sceneView, -float( deltaX ) / 20, -float( deltaY ) / 20 );
        }

        if (isMiddleMouseDown)
        {
            moveDir.x = -deltaX / 20.0f;
            moveDir.y = deltaY / 20.0f;
        }
        
        Inspector::Command inspectorCommand{};

        svMoveCamera( sceneView, moveDir + gamepadMoveDir );
        gamepadMoveDir = Vec3( 0, 0, 0 );

        if (event.type == WindowEventType::MouseWheelScrollDown || event.type == WindowEventType::MouseWheelScrollUp)
        {
            moveDir = {};
        }

        svBeginRender( sceneView, inspector.IsSSAOEnabled() ? SSAO::Enabled : SSAO::Disabled, inspector.IsBloomEnabled() ? Bloom::Enabled : Bloom::Disabled, inspector.GetBloomThreshold(), inspector.GetBloomIntensity() );
        svDrawSprites( sceneView, width, height );
        int goCount = 0;
        GameObject** gameObjects = svGetGameObjects( sceneView, goCount );
        int inspectorSelectedGameObject = 0;
        inspector.Render( width, height, selectedGO, inspectorCommand, gameObjects, goCount, svGetMaterial( sceneView ), inspectorSelectedGameObject );
        svEndRender( sceneView );

        if (inspectorSelectedGameObject >= 0)
        {
            selectedGO = svSelectGameObjectIndex( sceneView, inspectorSelectedGameObject );
        }
        
        switch (inspectorCommand)
        {
        case Inspector::Command::CreateGO:
            svAddGameObject( sceneView );
            break;
        case Inspector::Command::OpenScene:
        {
            char path[ 1024 ] = {};
            GetOpenPath( path, "scene" );
            auto contents = FileSystem::FileContents( path );
            svLoadScene( sceneView, contents );
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
            ofn.lpstrFilter = "Scene\0*.SCENE\0All\0*.*\0";
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
            FILE* f = popen( "zenity --file-selection --save --title \"Save .scene file\"", "r" );

            if (!f)
            {
                System::Print( "Could not open file for saving.\n" );
                break;
            }
            
            fgets( path, 1024, f );
            pclose( f );
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
