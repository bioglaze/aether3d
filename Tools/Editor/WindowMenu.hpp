#ifndef WINDOWMENU_H
#define WINDOWMENU_H

#include <QWidget>

class QAction;
class QMenu;
class QMenuBar;

class WindowMenu : QObject
{
    Q_OBJECT

public:
    WindowMenu();
    ~WindowMenu();
    void Init( QWidget* mainWindow );
    QMenuBar* menuBar = nullptr;

private:
    QMenu* fileMenu = nullptr;
    QMenu* sceneMenu = nullptr;
    QAction* loadSceneAction = nullptr;
    QAction* createCameraAction = nullptr;
    QAction* createGoAction = nullptr;
};

#endif
