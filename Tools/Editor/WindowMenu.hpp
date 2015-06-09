#ifndef WINDOWMENU_H
#define WINDOWMENU_H

#include <QObject>

class QMenu;
class QMenuBar;
class QWidget;

class WindowMenu : QObject
{
    Q_OBJECT

public:
    void Init( QWidget* mainWindow );
    QMenuBar* menuBar = nullptr;

private:
    QMenu* fileMenu = nullptr;
    QMenu* sceneMenu = nullptr;
    QMenu* editMenu = nullptr;
    QMenu* componentMenu = nullptr;
};

#endif
