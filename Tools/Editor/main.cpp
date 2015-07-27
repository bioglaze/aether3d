#include <QApplication>
#include "MainWindow.hpp"

// TODO: delete all selected game objects.
// FIXME: deleting gameobjects cause memory corruption that does not crash instantly.

int main( int argc, char *argv[] )
{
    QApplication a( argc, argv );
    a.setKeyboardInputInterval( 10 );

    MainWindow mainWindow;
    // Prevents a warning on Windows.
    mainWindow.setGeometry(200,200,200,200);
    mainWindow.showMaximized();
    a.installEventFilter( (QObject*)mainWindow.GetSceneWidget() );

    return a.exec();
}

