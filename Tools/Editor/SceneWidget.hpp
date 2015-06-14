#ifndef SCENEWIDGET_HPP
#define SCENEWIDGET_HPP

#include <memory>
#include <list>
#include <vector>
#include <QOpenGLWidget>
#include "GameObject.hpp"
#include "CameraComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "Texture2D.hpp"
#include "Scene.hpp"

class SceneWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit SceneWidget( QWidget* parent = 0 );

    void Init();

    /// \return Index of created game object.
    /// Should only be called by CreateGoCommand!
    ae3d::GameObject* CreateGameObject();

    /// \param index Index.
    /// TODO: Implement as a command.
    void RemoveGameObject( int index );

    int GetGameObjectCount() const { return gameObjects.size(); }

    //bool IsGameObjectInScene( int index ) const { return gameObjectsInScene[ index ] != 0; }

    ae3d::Scene* GetScene() { return &scene; }

    ae3d::GameObject* GetGameObject( int index ) { return gameObjects[ index ].get(); }

    // TODO: Maybe create an editor state class and put this there.
    std::list< int > selectedGameObjects;

protected:
    void initializeGL();
    void paintGL();
    void resizeGL( int width, int height );
    void updateGL();
    void keyPressEvent( QKeyEvent* event );
    void keyReleaseEvent( QKeyEvent* event );
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent( QMouseEvent* event );
    void mouseReleaseEvent( QMouseEvent* event );
    bool eventFilter(QObject *obj, QEvent *event);

private:
    ae3d::GameObject camera;
    ae3d::Texture2D spriteTex;
    ae3d::Scene scene;
    ae3d::GameObject spriteContainer;
    std::vector< std::shared_ptr< ae3d::GameObject > > gameObjects;
    //std::vector< int > gameObjectsInScene; // Using int to avoid bool vector specialization madness.
};

#endif
