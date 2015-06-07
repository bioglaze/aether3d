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

private slots:
    void LoadScene();

private:
    QMenu* fileMenu = nullptr;
    QAction* loadSceneAction = nullptr;
};

#endif // WINDOWMENU_H
