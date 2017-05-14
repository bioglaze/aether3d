#ifndef WINDOWMENU_H
#define WINDOWMENU_H

#include <list>
#include <QObject>

namespace ae3d
{
    class GameObject;
}

class WindowMenu : QObject
{
    Q_OBJECT

public:
    void Init( class QWidget* mainWindow );
    void SetUndoText( const char* text );
    void Update();
    class QMenuBar* menuBar = nullptr;

private slots:
    void GameObjectSelected( std::list< ae3d::GameObject* > gameObjects );

private:
    std::list< ae3d::GameObject* > gameObjects;
    class QMenu* fileMenu = nullptr;
    QMenu* sceneMenu = nullptr;
    QMenu* editMenu = nullptr;
    QMenu* componentMenu = nullptr;
    class QAction* undo = nullptr;
    QAction* actionAddMesh = nullptr;
    QAction* actionAddAudioSource = nullptr;
    QAction* actionAddSpriteRenderer = nullptr;
    QAction* actionAddCamera = nullptr;
    QAction* actionAddDirLight = nullptr;
    QAction* actionAddSpotLight = nullptr;
    QAction* actionAddPointLight = nullptr;
};

#endif
