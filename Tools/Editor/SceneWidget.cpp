#include "SceneWidget.hpp"
#include <QKeyEvent>
#include "System.hpp"
#include "FileSystem.hpp"
#include <qdir.h>
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

SceneWidget::SceneWidget(const QGLFormat & format, QWidget *parent) : QGLWidget(format, parent)
{
}

void SceneWidget::Init()
{
    System::InitGfxDeviceForEditor( width(), height() );
    System::LoadBuiltinAssets();
System::Print("dimension: %dx%d\n", width(), height());
    camera.AddComponent<CameraComponent>();
    camera.GetComponent<CameraComponent>()->SetProjection( 0, (float)width(), (float)height(), 0, 0, 1 );
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 1.0f, 0.5f, 0.5f ) );

    spriteTex.Load( FileSystem::FileContents( AbsoluteFilePath("glider.png").c_str() ), TextureWrap::Repeat, TextureFilter::Nearest, Mipmaps::None );

    spriteContainer.AddComponent<SpriteRendererComponent>();
    auto sprite = spriteContainer.GetComponent<SpriteRendererComponent>();
    sprite->SetTexture( &spriteTex, Vec3( 20, 0, -0.6f ), Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), Vec4( 1, 0.5f, 0.5f, 1 ) );
    spriteContainer.AddComponent<TransformComponent>();

    scene.Add( &camera );
    scene.Add( &spriteContainer );
    System::Print("inited widget\n");
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
    glClearColor( 1, 0, 0, 1 );
    glClear( GL_COLOR_BUFFER_BIT );
    scene.Render();
}

void SceneWidget::resizeGL( int width, int height )
{
    System::InitGfxDeviceForEditor( width, height );
    camera.GetComponent<CameraComponent>()->SetProjection( 0, (float)width, (float)height, 0, 0, 1 );
}

void SceneWidget::keyPressEvent( QKeyEvent* aEvent )
{
    ae3d::System::Print("key press\n");
    if (aEvent->key() == Qt::Key_Escape)
    {
        //mainWindow->SetSelectedNode( nullptr );
    }
    else if (aEvent->key() == Qt::Key_A)
    {
        //cameraMoveDir.x = -1;
        //ae3d::System::Print("Pressed A\n");
    }
 }

void SceneWidget::keyReleaseEvent( QKeyEvent* aEvent )
{

}

void SceneWidget::mousePressEvent( QMouseEvent* event )
{
    //ae3d::System::Print("Mouse press\n");
}

void SceneWidget::mouseReleaseEvent( QMouseEvent* event )
{
    //ae3d::System::Print("Mouse press\n");
    setFocus();
}

bool SceneWidget::eventFilter(QObject */*obj*/, QEvent *event)
{
    if (event->type() == QEvent::MouseMove)
    {
        //ae3d::System::Print("mouse move\n");
    }
}

void SceneWidget::wheelEvent(QWheelEvent *event)
{

}

