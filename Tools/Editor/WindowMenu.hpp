#ifndef WINDOWMENU_H
#define WINDOWMENU_H

#include <QObject>

class QMenu;
class QMenuBar;
class QWidget;

namespace ae3d
{
    class GameObject;
}

class WindowMenu : QObject
{
    Q_OBJECT

public:
    void Init( QWidget* mainWindow );
    QMenuBar* menuBar = nullptr;

private slots:
    void GameObjectSelected( ae3d::GameObject* gameObject );

private:
    QMenu* fileMenu = nullptr;
    QMenu* sceneMenu = nullptr;
    QMenu* editMenu = nullptr;
    QMenu* componentMenu = nullptr;
};

#endif
