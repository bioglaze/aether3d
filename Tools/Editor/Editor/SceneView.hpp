#pragma once

namespace ae3d
{
    class GameObject;
    class Material;
    struct Vec3;
    
    namespace FileSystem
    {
        struct FileContentsData;
    }
}

struct SceneView;

enum class SSAO
{
   Enabled,
   Disabled,
};

enum class Bloom
{
   Enabled,
   Disabled,
};

enum class CollisionFilter
{
    All,
    ExcludeGizmo,
    OnlyGizmo,
};

float svGetCameraSign( SceneView* sv );
void svInit( SceneView** sceneView, int width, int height );
bool svIsDraggingGizmo( SceneView* sv );
void svAddGameObject( SceneView* sceneView );
void svDuplicateGameObject( SceneView* sceneView );
void svDrawSprites( SceneView* sv, unsigned screenWidth, unsigned screenHeight );
void svBeginRender( SceneView* sceneView, SSAO ssao, Bloom bloom, float bloomThreshold, float bloomIntensity );
void svEndRender( SceneView* sceneView );
ae3d::Material* svGetMaterial( SceneView* sceneView );
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
ae3d::GameObject* svSelectGameObjectIndex( SceneView* sceneView, int index );
void svDeleteGameObject( SceneView* sceneView );
void svFocusOnSelected( SceneView* sceneView );
void svUpdate( SceneView* sceneView );
void svHighlightGizmo( SceneView* sv, int screenX, int screenY, int width, int height );
