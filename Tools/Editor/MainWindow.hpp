#ifndef MAINWINDOW
#define MAINWINDOW

#include <QMainWindow>
#include "WindowMenu.hpp"

class SceneWidget;
class QTreeWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    SceneWidget* GetSceneWidget() { return sceneWidget; }

private:
    void UpdateHierarchy();

    QTreeWidget* sceneTree = nullptr;
    SceneWidget* sceneWidget = nullptr;
    WindowMenu windowMenu;
};

#endif // MAINWINDOW

