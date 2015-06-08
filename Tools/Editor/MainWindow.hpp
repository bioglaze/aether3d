#ifndef MAINWINDOW
#define MAINWINDOW

#include <QMainWindow>
#include "WindowMenu.hpp"
#include "CommandManager.hpp"

class SceneWidget;
class QTreeWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    SceneWidget* GetSceneWidget() { return sceneWidget; }

public slots:
    void LoadScene();
    void CommandCreateCamera();
    void CommandCreateGameObject();

private:
    void UpdateHierarchy();

    QTreeWidget* sceneTree = nullptr;
    SceneWidget* sceneWidget = nullptr;
    WindowMenu windowMenu;
    CommandManager commandManager;
};

#endif

