#include "SceneWidget.hpp"
#include <QKeyEvent>
#include <QSurfaceFormat>
#include <QApplication>
#include <QDir>
#include <QGLFormat>
#include <QMessageBox>
#include "System.hpp"
#include "FileSystem.hpp"
#include "TransformComponent.hpp"
#include "MeshRendererComponent.hpp"

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

    spriteTex.Load( FileSystem::FileContents( AbsoluteFilePath("glider.png").c_str() ), TextureWrap::Repeat, TextureFilter::Nearest, Mipmaps::None, 1 );

    cubeMesh.Load( FileSystem::FileContents( AbsoluteFilePath( "textured_cube.ae3d" ).c_str() ) );
    cubeContainer.AddComponent< MeshRendererComponent >();
    cubeContainer.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    cubeContainer.AddComponent< TransformComponent >();
    cubeContainer.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 0, -50 } );

    cubeShader.Load( FileSystem::FileContents( AbsoluteFilePath("unlit.vsh").c_str() ), FileSystem::FileContents( AbsoluteFilePath("unlit.fsh").c_str() ), "unlitVert", "unlitFrag" );

    cubeMaterial.SetShader( &cubeShader );
    cubeMaterial.SetTexture( "textureMap", &spriteTex );
    cubeMaterial.SetVector( "tint", { 1, 1, 1, 1 } );
    cubeMaterial.SetBackFaceCulling( true );

    cubeContainer.GetComponent< MeshRendererComponent >()->SetMaterial( &cubeMaterial, 0 );

    scene.Add( &camera );
    scene.Add( &cubeContainer );

    connect( &myTimer, SIGNAL( timeout() ), this, SLOT( UpdateCamera() ) );
    myTimer.start();
    connect(mainWindow, SIGNAL(GameObjectSelected(std::list< ae3d::GameObject* >)),
            this, SLOT(GameObjectSelected(std::list< ae3d::GameObject* >)));
    //new QShortcut(QKeySequence("Home"), this, SLOT(resetView()));
    //new QShortcut(QKeySequence("Ctrl+Tab"), this, SLOT(togglePreview()));
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
    if (aEvent->key() == Qt::Key_Escape)
    {
        //mainWindow->SetSelectedNode( nullptr );
    }
    else if (aEvent->key() == Qt::Key_A && mouseMode == MouseMode::Grab)
    {
        cameraMoveDir.x = -1;
    }
    else if (aEvent->key() == Qt::Key_D && mouseMode == MouseMode::Grab)
    {
        cameraMoveDir.x = 1;
    }
    else if (aEvent->key() == Qt::Key_Q && mouseMode == MouseMode::Grab)
    {
        cameraMoveDir.y = -1;
    }
    else if (aEvent->key() == Qt::Key_E && mouseMode == MouseMode::Grab)
    {
        cameraMoveDir.y = 1;
    }
    else if (aEvent->key() == Qt::Key_W && mouseMode == MouseMode::Grab)
    {
        cameraMoveDir.z = -1;
    }
    else if (aEvent->key() == Qt::Key_S && mouseMode == MouseMode::Grab)
    {
        cameraMoveDir.z = 1;
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
    /*else if (aEvent->key() == Qt::Key_F)
    {
        if (editorState->selectedNode)
        {
            CenterSelected();
        }
    }*/
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
}

void SceneWidget::mouseReleaseEvent( QMouseEvent* /*event*/ )
{
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
    // TODO: Highlight etc.
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
