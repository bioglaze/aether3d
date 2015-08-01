#include "SceneWidget.hpp"
#include <vector>
#include <cmath>
#include <QKeyEvent>
#include <QSurfaceFormat>
#include <QApplication>
#include <QDir>
#include <QGLFormat>
#include <QMessageBox>
#include "System.hpp"
#include "FileSystem.hpp"
#include "TransformComponent.hpp"
#include "Matrix.hpp"
#include "MeshRendererComponent.hpp"
#include "Mesh.hpp"
#include "Vec3.hpp"

#include <iostream>

using namespace ae3d;

std::string AbsoluteFilePath( const std::string& relativePath )
{
    QDir dir = QDir::currentPath();
#if __APPLE__
    // On OS X the executable is inside .app, so this gets a path outside it.
    dir.cdUp();
    dir.cdUp();
    dir.cdUp();
#endif
    return dir.absoluteFilePath( relativePath.c_str() ).toUtf8().constData();
}

void ScreenPointToRay( int screenX, int screenY, float screenWidth, float screenHeight, GameObject& camera, Vec3& outRayOrigin, Vec3& outRayTarget )
{
    const float aspect = screenHeight / screenWidth;
    const float halfWidth = screenWidth * 0.5f;
    const float halfHeight = screenHeight * 0.5f;
    const float fov = camera.GetComponent< CameraComponent >()->GetFovDegrees() * (3.1415926535f / 180.0f);

    // Normalizes screen coordinates and scales them to the FOV.
    const float dx = std::tan( fov * 0.5f ) * (screenX / halfWidth - 1.0f) / aspect;
    const float dy = std::tan( fov * 0.5f ) * (screenY / halfHeight - 1.0f);

    Matrix44 view;
    camera.GetComponent< TransformComponent >()->GetLocalRotation().GetMatrix( view );
    Matrix44 translation;
    translation.Translate( -camera.GetComponent< TransformComponent >()->GetLocalPosition() );
    Matrix44::Multiply( translation, view, view );

    Matrix44 invView;
    Matrix44::Invert( view, invView );

    const float farp = camera.GetComponent< CameraComponent >()->GetFar();

    outRayOrigin = camera.GetComponent< TransformComponent >()->GetLocalPosition();
    outRayTarget = -Vec3( -dx * farp, dy * farp, farp );

    Matrix44::TransformPoint( outRayTarget, invView, &outRayTarget );
}

template< typename T >
T Min2( T t1, T t2 )
{
    return t1 < t2 ? t1 : t2;
}

template< typename T >
T Max2( T t1, T t2 )
{
    return t1 > t2 ? t1 : t2;
}

float IntersectRayAABB( const Vec3& origin, const Vec3& target, const Vec3& min, const Vec3& max )
{
    const Vec3 dir = (target - origin).Normalized();

    Vec3 dirfrac;
    dirfrac.x = 1.0f / dir.x;
    dirfrac.y = 1.0f / dir.y;
    dirfrac.z = 1.0f / dir.z;

    const float t1 = (min.x - origin.x) * dirfrac.x;
    const float t2 = (max.x - origin.x) * dirfrac.x;
    const float t3 = (min.y - origin.y) * dirfrac.y;
    const float t4 = (max.y - origin.y) * dirfrac.y;
    const float t5 = (min.z - origin.z) * dirfrac.z;
    const float t6 = (max.z - origin.z) * dirfrac.z;

    const float tmin = Max2< float >( Max2< float >( Min2< float >(t1, t2), Min2< float >(t3, t4)), Min2< float >(t5, t6) );
    const float tmax = Min2< float >( Min2< float >( Max2< float >(t1, t2), Max2< float >(t3, t4)), Max2< float >(t5, t6) );

    // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
    if (tmax < 0)
    {
        return -1.0f;
    }

    // if tmin > tmax, ray doesn't intersect AABB
    if (tmin > tmax)
    {
        return -1.0f;
    }

    return tmin;
}

struct CollisionInfo
{
    GameObject* go;
    std::vector< int > subMeshIndices;
};

SceneWidget::GizmoAxis CollidesWithGizmo( GameObject& camera, GameObject& gizmo, int screenX, int screenY, int width, int height, float maxDistance )
{
    Vec3 rayOrigin, rayTarget;
    ScreenPointToRay( screenX, screenY, width, height, camera, rayOrigin, rayTarget );

    auto meshRenderer = gizmo.GetComponent< MeshRendererComponent >();
    auto meshLocalToWorld = gizmo.GetComponent< TransformComponent >() ? gizmo.GetComponent< TransformComponent >()->GetLocalMatrix() : Matrix44::identity;

    Vec3 oMin, oMax;
    Matrix44::TransformPoint( meshRenderer->GetMesh()->GetAABBMin(), meshLocalToWorld, &oMin );
    Matrix44::TransformPoint( meshRenderer->GetMesh()->GetAABBMax(), meshLocalToWorld, &oMax );

    const float meshDistance = IntersectRayAABB( rayOrigin, rayTarget, oMin, oMax );

    if (meshDistance < 0 || meshDistance > maxDistance)
    {
        return SceneWidget::GizmoAxis::None;
    }

    Vec3 upMin, upMax;
    Matrix44::TransformPoint( meshRenderer->GetMesh()->GetSubMeshAABBMin( 2 ), meshLocalToWorld, &upMin );
    Matrix44::TransformPoint( meshRenderer->GetMesh()->GetSubMeshAABBMax( 2 ), meshLocalToWorld, &upMax );
    const float upDistance = IntersectRayAABB( rayOrigin, rayTarget, upMin, upMax );

    if (-1 < upDistance && upDistance < maxDistance)
    {
        return SceneWidget::GizmoAxis::Y;
    }

    Vec3 rightMin, rightMax;
    Matrix44::TransformPoint( meshRenderer->GetMesh()->GetSubMeshAABBMin( 1 ), meshLocalToWorld, &rightMin );
    Matrix44::TransformPoint( meshRenderer->GetMesh()->GetSubMeshAABBMax( 1 ), meshLocalToWorld, &rightMax );
    const float rightDistance = IntersectRayAABB( rayOrigin, rayTarget, rightMin, rightMax );

    if (-1 < rightDistance && rightDistance < maxDistance)
    {
        return SceneWidget::GizmoAxis::X;
    }

    Vec3 forwardMin, forwardMax;
    Matrix44::TransformPoint( meshRenderer->GetMesh()->GetSubMeshAABBMin( 0 ), meshLocalToWorld, &forwardMin );
    Matrix44::TransformPoint( meshRenderer->GetMesh()->GetSubMeshAABBMax( 0 ), meshLocalToWorld, &forwardMax );
    const float forwardDistance = IntersectRayAABB( rayOrigin, rayTarget, forwardMin, forwardMax );

    if (-1 < forwardDistance && forwardDistance < maxDistance)
    {
        return SceneWidget::GizmoAxis::Z;
    }

    //std::cout << "gizmo distance: " << meshDistance << std::endl;
    return SceneWidget::GizmoAxis::None;
}

std::vector< CollisionInfo > GetColliders( GameObject& camera, const std::vector< std::shared_ptr< ae3d::GameObject > >& gameObjects,
                                           int screenX, int screenY, int width, int height, float maxDistance )
{
    Vec3 rayOrigin, rayTarget;
    ScreenPointToRay( screenX, screenY, width, height, camera, rayOrigin, rayTarget );
    std::vector< CollisionInfo > outInfo;

    // Collects meshes that collide with the ray.
    for (auto& go : gameObjects)
    {
        //std::cout << "Looping game objects" << std::endl;
        auto meshRenderer = go->GetComponent< MeshRendererComponent >();

        if (!meshRenderer || !meshRenderer->GetMesh())
        {
            continue;
        }

        auto meshLocalToWorld = go->GetComponent< TransformComponent >() ? go->GetComponent< TransformComponent >()->GetLocalMatrix() : Matrix44::identity;
        Vec3 oMin, oMax;
        Matrix44::TransformPoint( meshRenderer->GetMesh()->GetAABBMin(), meshLocalToWorld, &oMin );
        Matrix44::TransformPoint( meshRenderer->GetMesh()->GetAABBMax(), meshLocalToWorld, &oMax );

        const float meshDistance = IntersectRayAABB( rayOrigin, rayTarget, oMin, oMax );
        //std::cout << "mesh distance: " << meshDistance << std::endl;
    }

    return outInfo;
}

void SceneWidget::TransformGizmo::Init( Shader* shader )
{
    translateTex.Load( FileSystem::FileContents( AbsoluteFilePath("glider.png").c_str() ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::None, 1 );
    translateMesh.Load( FileSystem::FileContents( AbsoluteFilePath( "cursor_translate.ae3d" ).c_str() ) );

    xAxisMaterial.SetShader( shader );
    xAxisMaterial.SetTexture( "textureMap", &translateTex );
    xAxisMaterial.SetVector( "tint", { 1, 0, 0, 1 } );
    xAxisMaterial.SetBackFaceCulling( true );

    yAxisMaterial.SetShader( shader );
    yAxisMaterial.SetTexture( "textureMap", &translateTex );
    yAxisMaterial.SetVector( "tint", { 0, 1, 0, 1 } );
    yAxisMaterial.SetBackFaceCulling( true );

    zAxisMaterial.SetShader( shader );
    zAxisMaterial.SetTexture( "textureMap", &translateTex );
    zAxisMaterial.SetVector( "tint", { 0, 0, 1, 1 } );
    zAxisMaterial.SetBackFaceCulling( true );

    go.AddComponent< MeshRendererComponent >();
    go.GetComponent< MeshRendererComponent >()->SetMesh( &translateMesh );
    go.GetComponent< MeshRendererComponent >()->SetMaterial( &xAxisMaterial, 1 );
    go.GetComponent< MeshRendererComponent >()->SetMaterial( &yAxisMaterial, 2 );
    go.GetComponent< MeshRendererComponent >()->SetMaterial( &zAxisMaterial, 0 );
    go.AddComponent< TransformComponent >();
    go.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 10, -50 } );
}

void SceneWidget::TransformGizmo::SetPosition( const ae3d::Vec3& position )
{
    go.GetComponent< TransformComponent >()->SetLocalPosition( position );
}

SceneWidget::SceneWidget( QWidget* parent ) : QOpenGLWidget( parent )
{
    if (!QGLFormat::hasOpenGL())
    {
        QMessageBox::critical(this,
                        "No OpenGL Support",
                        "Missing OpenGL support! Maybe you need to update your display driver.");
    }

    QSurfaceFormat fmt;
#if __APPLE__
    fmt.setVersion( 4, 1 );
#else
    fmt.setVersion( 4, 4 );
#endif
    //fmt.setDepthBufferSize(24);
    fmt.setProfile( QSurfaceFormat::CoreProfile );
    setFormat( fmt );
    QSurfaceFormat::setDefaultFormat( fmt );
}

void SceneWidget::Init()
{
    System::Assert( mainWindow != nullptr, "mainWindow not set.");

    System::InitGfxDeviceForEditor( width(), height() );
    System::LoadBuiltinAssets();

    camera.AddComponent<CameraComponent>();
    camera.GetComponent<CameraComponent>()->SetProjection( 45, float( width() ) / height(), 1, 400 );
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0, 0, 0 ) );
    camera.AddComponent<TransformComponent>();
    camera.GetComponent<TransformComponent>()->LookAt( { 0, 0, 0 }, { 0, 0, -100 }, { 0, 1, 0 } );

    spriteTex.Load( FileSystem::FileContents( AbsoluteFilePath("glider.png").c_str() ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::None, 1 );

    cubeMesh.Load( FileSystem::FileContents( AbsoluteFilePath( "textured_cube.ae3d" ).c_str() ) );

    gameObjects.push_back( std::make_shared< GameObject >() );
    gameObjects[ 0 ]->AddComponent< MeshRendererComponent >();
    gameObjects[ 0 ]->GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    gameObjects[ 0 ]->AddComponent< TransformComponent >();
    gameObjects[ 0 ]->GetComponent< TransformComponent >()->SetLocalPosition( { 0, 0, -50 } );
    gameObjects[ 0 ]->SetName( "Game Object" );

    unlitShader.Load( FileSystem::FileContents( AbsoluteFilePath("unlit.vsh").c_str() ), FileSystem::FileContents( AbsoluteFilePath("unlit.fsh").c_str() ), "unlitVert", "unlitFrag" );

    cubeMaterial.SetShader( &unlitShader );
    cubeMaterial.SetTexture( "textureMap", &spriteTex );
    cubeMaterial.SetVector( "tint", { 1, 1, 1, 1 } );
    cubeMaterial.SetBackFaceCulling( true );

    gameObjects[ 0 ]->GetComponent< MeshRendererComponent >()->SetMaterial( &cubeMaterial, 0 );

    transformGizmo.Init( &unlitShader );

    AddEditorObjects();
    scene.Add( &camera );
    scene.Add( gameObjects[ 0 ].get() );
    //scene.Add( &transformGizmo.go );

    connect( &myTimer, SIGNAL( timeout() ), this, SLOT( UpdateCamera() ) );
    myTimer.start();
    connect(mainWindow, SIGNAL(GameObjectSelected(std::list< ae3d::GameObject* >)),
            this, SLOT(GameObjectSelected(std::list< ae3d::GameObject* >)));

    //new QShortcut(QKeySequence("Home"), this, SLOT(resetView()));
    //new QShortcut(QKeySequence("Ctrl+Tab"), this, SLOT(togglePreview()));
}

void SceneWidget::RemoveEditorObjects()
{
    scene.Remove( &camera );
}

void SceneWidget::AddEditorObjects()
{
    scene.Add( &camera );
}

void SceneWidget::initializeGL()
{
    Init();
}

void SceneWidget::updateGL()
{
    update();
}

void SceneWidget::paintGL()
{
    scene.Render();
}

void SceneWidget::resizeGL( int width, int height )
{
    System::InitGfxDeviceForEditor( width, height );
    camera.GetComponent<CameraComponent>()->SetProjection( 45, float( width ) / height, 1, 400 );
}

void SceneWidget::keyPressEvent( QKeyEvent* aEvent )
{
    const bool shift = aEvent->modifiers() & Qt::ShiftModifier;
    const float speed = shift ? 2 : 1;

    if (aEvent->key() == Qt::Key_Escape)
    {
        selectedGameObjects.clear();
        emit GameObjectsAddedOrDeleted();
    }
    else if (aEvent->key() == Qt::Key_A && mouseMode == MouseMode::Grab)
    {
        cameraMoveDir.x = -speed;
    }
    else if (aEvent->key() == Qt::Key_D && mouseMode == MouseMode::Grab)
    {
        cameraMoveDir.x = speed;
    }
    else if (aEvent->key() == Qt::Key_Q && mouseMode == MouseMode::Grab)
    {
        cameraMoveDir.y = -speed;
    }
    else if (aEvent->key() == Qt::Key_E && mouseMode == MouseMode::Grab)
    {
        cameraMoveDir.y = speed;
    }
    else if (aEvent->key() == Qt::Key_W && mouseMode == MouseMode::Grab)
    {
        cameraMoveDir.z = -speed;
    }
    else if (aEvent->key() == Qt::Key_S && mouseMode == MouseMode::Grab)
    {
        cameraMoveDir.z = speed;
    }
 }

void SceneWidget::keyReleaseEvent( QKeyEvent* aEvent )
{
    if (aEvent->key() == Qt::Key_A)
    {
        cameraMoveDir.x = 0;
    }
    else if (aEvent->key() == Qt::Key_D)
    {
        cameraMoveDir.x = 0;
    }
    else if (aEvent->key() == Qt::Key_Q)
    {
        cameraMoveDir.y = 0;
    }
    else if (aEvent->key() == Qt::Key_E)
    {
        cameraMoveDir.y = 0;
    }
    else if (aEvent->key() == Qt::Key_W)
    {
        cameraMoveDir.z = 0;
    }
    else if (aEvent->key() == Qt::Key_S)
    {
        cameraMoveDir.z = 0;
    }
    else if (aEvent->key() == Qt::Key_F)
    {
        /*if (editorState->selectedNode)
        {
            CenterSelected();
        }*/
    }
}

void SceneWidget::mousePressEvent( QMouseEvent* event )
{
    setFocus();

    if (event->button() == Qt::RightButton && mouseMode != MouseMode::Grab)
    {
        mouseMode = MouseMode::Grab;
        setCursor( Qt::BlankCursor );
        lastMousePosition[ 0 ] = event->x();
        lastMousePosition[ 1 ] = event->y();
    }
    else if (event->button() == Qt::MiddleButton)
    {
        mouseMode = MouseMode::Pan;
        lastMousePosition[ 0 ] = event->x();
        lastMousePosition[ 1 ] = event->y();
        cursor().setShape( Qt::ClosedHandCursor );
    }
    else if (event->button() == Qt::LeftButton)
    {
        dragAxis = CollidesWithGizmo( camera, transformGizmo.go, event->x(), event->y(), width(), height(), 200 );
    }
}

void SceneWidget::mouseReleaseEvent( QMouseEvent* event )
{
    dragAxis = GizmoAxis::None;

    if (mouseMode == MouseMode::Grab)
    {
        mouseMode = MouseMode::Normal;
        unsetCursor();
    }
    else if (mouseMode == MouseMode::Pan)
    {
        cameraMoveDir.x = 0;
        cameraMoveDir.y = 0;
        mouseMode = MouseMode::Normal;
        cursor().setShape( Qt::ArrowCursor );
    }

    if (event->button() == Qt::LeftButton)
    {
        const QPoint point = mapFromGlobal( QCursor::pos() );
        auto colliders = GetColliders( camera, gameObjects, point.x(), point.y(), width(), height(), 200 );
    }
}

bool SceneWidget::eventFilter( QObject * /*obj*/, QEvent *event )
{
    if (event->type() == QEvent::MouseMove)
    {
        const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        float deltaX = (lastMousePosition[ 0 ] - mouseEvent->x()) * 0.1f;
        float deltaY = (lastMousePosition[ 1 ] - mouseEvent->y()) * 0.1f;

        if (mouseMode == MouseMode::Grab)
        {
            QPoint globalPos = mapToGlobal(QPoint(mouseEvent->x(), mouseEvent->y()));

            int x = mouseEvent->x();
            int y = mouseEvent->y();

            if (globalPos.x() < 5)
            {
                x = desktop.geometry().width() - 10;
                cursor().setPos( desktop.geometry().width() - 10, globalPos.y() );
            }
            else if (globalPos.x() > desktop.geometry().width() - 5)
            {
                x = 10;
                cursor().setPos( 10, globalPos.y() );
            }

            if (globalPos.y() < 5)
            {
                cursor().setPos( globalPos.x(), desktop.geometry().height() - 10 );
            }
            else if (globalPos.y() > desktop.geometry().height() - 10 )
            {
                cursor().setPos( globalPos.x(), 10 );
            }

            deltaX = deltaX > 5 ? 5 : deltaX;
            deltaX = deltaX < -5 ? -5 : deltaX;

            deltaY = deltaY > 5 ? 5 : deltaY;
            deltaY = deltaY < -5 ? -5 : deltaY;

            camera.GetComponent< ae3d::TransformComponent >()->OffsetRotate( Vec3( 0.0f, 1.0f, 0.0f ), deltaX );
            camera.GetComponent< ae3d::TransformComponent >()->OffsetRotate( Vec3( 1.0f, 0.0f, 0.0f ), deltaY );

            lastMousePosition[ 0 ] = x;
            lastMousePosition[ 1 ] = y;
            return true;
        }
        else if (mouseMode == MouseMode::Pan)
        {
            cameraMoveDir.x = deltaX * 0.1f;
            cameraMoveDir.y = -deltaY * 0.1f;
            camera.GetComponent< ae3d::TransformComponent >()->MoveRight( cameraMoveDir.x );
            camera.GetComponent< ae3d::TransformComponent >()->MoveUp( cameraMoveDir.y );
            cameraMoveDir.x = cameraMoveDir.y = 0;
            lastMousePosition[ 0 ] = mouseEvent->x();
            lastMousePosition[ 1 ] = mouseEvent->y();
            return true;
        }
        else if (dragAxis == GizmoAxis::X || dragAxis == GizmoAxis::Y || dragAxis == GizmoAxis::Z)
        {
            const Vec3 direction = camera.GetComponent<TransformComponent>()->GetViewDirection();

            for (auto goIndex : selectedGameObjects)
            {
                const Vec3 axisMask { dragAxis == GizmoAxis::X ? 1.0f : 0.0f, dragAxis == GizmoAxis::Y ? 1.0f : 0.0f, dragAxis == GizmoAxis::Z ? 1.0f : 0.0f };
                GameObject* go = gameObjects[ goIndex ].get();
                const Vec3 oldPosition = go->GetComponent< TransformComponent >()->GetLocalPosition();

                float xOffset = -deltaX;

                if (xOffset > 1)
                {
                    xOffset = 1;
                }
                if (xOffset < -1)
                {
                    xOffset = -1;
                }

                float zOffset = -xOffset;

                float yOffset = deltaY;

                if (yOffset > 1)
                {
                    yOffset = 1;
                }
                if (yOffset < -1)
                {
                    yOffset = -1;
                }

                const Vec3 newPosition = oldPosition + Vec3( xOffset * direction.z, yOffset, zOffset * direction.x ) * axisMask;
                go->GetComponent< TransformComponent >()->SetLocalPosition( newPosition );
                transformGizmo.SetPosition( newPosition );
                lastMousePosition[ 0 ] = mouseEvent->x();
                lastMousePosition[ 1 ] = mouseEvent->y();
            }
        }
        else if (!selectedGameObjects.empty())
        {
            GizmoAxis axis = CollidesWithGizmo( camera, transformGizmo.go, mouseEvent->x(), mouseEvent->y(), width(), height(), 200 );

            transformGizmo.xAxisMaterial.SetVector( "tint", axis == GizmoAxis::X ? Vec4( 1, 1, 1, 1 ) : Vec4( 1, 0, 0, 1 ) );
            transformGizmo.yAxisMaterial.SetVector( "tint", axis == GizmoAxis::Y ? Vec4( 1, 1, 1, 1 ) : Vec4( 0, 1, 0, 1 ) );
            transformGizmo.zAxisMaterial.SetVector( "tint", axis == GizmoAxis::Z ? Vec4( 1, 1, 1, 1 ) : Vec4( 0, 0, 1, 1 ) );
        }
    }
    else if (event->type() == QEvent::Quit)
    {
        QApplication::quit();
    }

    return false;
}

void SceneWidget::wheelEvent( QWheelEvent* event )
{
    const float dir = event->angleDelta().y() < 0 ? -1 : 1;
    const float speed = dir;
    camera.GetComponent< ae3d::TransformComponent >()->MoveForward( speed );
}

void SceneWidget::UpdateCamera()
{
    const float speed = 0.2f;
    camera.GetComponent< ae3d::TransformComponent >()->MoveRight( cameraMoveDir.x * speed );
    camera.GetComponent< ae3d::TransformComponent >()->MoveUp( cameraMoveDir.y * speed );
    camera.GetComponent< ae3d::TransformComponent >()->MoveForward( cameraMoveDir.z * speed );
    updateGL();
}

void SceneWidget::GameObjectSelected( std::list< ae3d::GameObject* > gameObjects )
{
    std::cout << "scene got selected event. go count: " << gameObjects.size() << std::endl;

    if (gameObjects.empty())
    {
        scene.Remove( &transformGizmo.go );
        return;
    }

    Vec3 avgPosition;

    for (auto go : gameObjects)
    {
        auto transform = go->GetComponent< ae3d::TransformComponent >();

        if (transform)
        {
            avgPosition += transform->GetLocalPosition();
        }
    }

    avgPosition /= gameObjects.size();

    transformGizmo.SetPosition( avgPosition );
    scene.Add( &transformGizmo.go );
}

ae3d::GameObject* SceneWidget::CreateGameObject()
{
    gameObjects.push_back( std::make_shared<ae3d::GameObject>() );
    gameObjects.back()->SetName( "Game Object" );
    gameObjects.back()->AddComponent< ae3d::TransformComponent >();
    scene.Add( gameObjects.back().get() );
    selectedGameObjects.clear();
    selectedGameObjects.push_back( (int)gameObjects.size() - 1 );
    return gameObjects.back().get();
}

void SceneWidget::RemoveGameObject( int index )
{
    scene.Remove( gameObjects[ index ].get() );
    gameObjects.erase( gameObjects.begin() + index );
}

void SceneWidget::LoadSceneFromFile( const char* path )
{
    std::vector< ae3d::GameObject > gos;
    Scene::DeserializeResult result = scene.Deserialize( ae3d::FileSystem::FileContents( path ), gos );

    if (result == Scene::DeserializeResult::ParseError)
    {
        QMessageBox::critical( this,
                        "Scene Parse Error", "There was an error parsing the scene. More info in console." );
    }

    gameObjects.clear();

    for (auto& go : gos)
    {
        gameObjects.push_back( std::make_shared< ae3d::GameObject >() );
        *gameObjects.back() = go;
        scene.Add( gameObjects.back().get() );
    }

    emit GameObjectsAddedOrDeleted();
}
