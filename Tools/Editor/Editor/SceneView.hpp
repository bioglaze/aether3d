#pragma once

namespace ae3d
{
    class GameObject;
    struct Vec3;
    
    namespace FileSystem
    {
        struct FileContentsData;
    }
}

struct SceneView;

void svInit( SceneView** sceneView, int width, int height );
void svAddGameObject( SceneView* sceneView );
void svDrawSprites( SceneView* sv, unsigned screenWidth, unsigned screenHeight );
void svBeginRender( SceneView* sceneView );
void svEndRender( SceneView* sceneView );
bool svIsTransformGizmoSelected( SceneView* sceneView );
ae3d::GameObject** svGetGameObjects( SceneView* sceneView, int& outCount );
void svLoadScene( SceneView* sceneView, const ae3d::FileSystem::FileContentsData& contents );
void svSaveScene( SceneView* sceneView, char* path );
void svRotateCamera( SceneView* sceneView, float xDegrees, float yDegrees );
void svMoveCamera( SceneView* sceneView, const ae3d::Vec3& moveDir );
void svHandleMouseMotion( SceneView* sv, int deltaX, int deltaY );
void svHandleLeftMouseDown( SceneView* sv, int screenX, int screenY, int width, int height );
void svHandleLeftMouseUp( SceneView* sv );
void svMoveSelection( SceneView* sceneView, const ae3d::Vec3& moveDir );
ae3d::GameObject* svSelectObject( SceneView* sceneView, int screenX, int screenY, int width, int height );
void svDeleteGameObject( SceneView* sceneView );
void svFocusOnSelected( SceneView* sceneView );
