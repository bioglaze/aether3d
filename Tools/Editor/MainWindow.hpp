#ifndef MAINWINDOW
#define MAINWINDOW

#include <QMainWindow>

class SceneWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    SceneWidget* GetSceneWidget() { return sceneWidget; }

private:
    SceneWidget* sceneWidget = nullptr;
};

#endif // MAINWINDOW

