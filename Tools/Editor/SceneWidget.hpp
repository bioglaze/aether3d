#ifndef SCENEWIDGET_HPP
#define SCENEWIDGET_HPP

#include <QtOpenGL/QGLWidget>
#include "GameObject.hpp"
#include "CameraComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "Texture2D.hpp"
#include "Scene.hpp"


class SceneWidget : public QGLWidget
{
    Q_OBJECT

public:
    explicit SceneWidget(const QGLFormat & format, QWidget *parent=0);
    explicit SceneWidget(QWidget *parent = 0) {}
    void Init();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL( int width, int height );
    void keyPressEvent( QKeyEvent* event );
    void keyReleaseEvent( QKeyEvent* event );
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent( QMouseEvent* event );
    void mouseReleaseEvent( QMouseEvent* event );
    void updateGL();
    bool eventFilter(QObject *obj, QEvent *event);

private:
    ae3d::GameObject camera;
    ae3d::Texture2D spriteTex;
    ae3d::Scene scene;
    ae3d::GameObject spriteContainer;
};

#endif // SCENEWIDGET_HPP
