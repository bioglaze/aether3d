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
    fmt.setVersion(3, 2);
#else
    fmt.setVersion(4, 3);
#endif
    //fmt.setDepthBufferSize(24);
    fmt.setProfile( QSurfaceFormat::CoreProfile );
    setFormat( fmt );
    QSurfaceFormat::setDefaultFormat( fmt );
}

void SceneWidget::Init()
{
    System::InitGfxDeviceForEditor( width(), height() );
    System::LoadBuiltinAssets();

    camera.AddComponent<CameraComponent>();
    camera.GetComponent<CameraComponent>()->SetProjection( 0, (float)width(), (float)height(), 0, 0, 1 );
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0, 0, 0  ) );

    spriteTex.Load( FileSystem::FileContents( AbsoluteFilePath("glider.png").c_str() ), TextureWrap::Repeat, TextureFilter::Nearest, Mipmaps::None );

    spriteContainer.AddComponent<SpriteRendererComponent>();
    auto sprite = spriteContainer.GetComponent<SpriteRendererComponent>();
    sprite->SetTexture( &spriteTex, Vec3( 20, 0, -0.6f ), Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), Vec4( 1, 1, 1, 1 ) );
    spriteContainer.AddComponent<TransformComponent>();

    scene.Add( &camera );
    scene.Add( &spriteContainer );

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
    camera.GetComponent<CameraComponent>()->SetProjection( 0, (float)width, (float)height, 0, 0, 1 );
}

void SceneWidget::keyPressEvent( QKeyEvent* aEvent )
{
    if (aEvent->key() == Qt::Key_Escape)
    {
        //mainWindow->SetSelectedNode( nullptr );
    }
    else if (aEvent->key() == Qt::Key_A)
    {
    }
 }

void SceneWidget::keyReleaseEvent( QKeyEvent* /*aEvent*/ )
{

}

void SceneWidget::mousePressEvent( QMouseEvent* /*event*/ )
{
}

void SceneWidget::mouseReleaseEvent( QMouseEvent* /*event*/ )
{
    setFocus();
}

bool SceneWidget::eventFilter( QObject * /*obj*/, QEvent *event )
{
    if (event->type() == QEvent::MouseMove)
    {
    }
    else if (event->type() == QEvent::Quit)
    {
        QApplication::quit();
    }

    return false;
}

void SceneWidget::wheelEvent( QWheelEvent * /*event*/)
{
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
