#include <QApplication>
#include "MainWindow.hpp"

// FIXME: deleting gameobjects causes memory corruption that does not crash instantly.
// TODO: delete all selected game objects.
// TODO: game object delete command
// TODO: game object rename command
// TODO: Ray-Triangle intersection for selection.
// TODO: Prevent empty name for game object

int main( int argc, char *argv[] )
{
    QApplication app( argc, argv );
    app.setKeyboardInputInterval( 10 );

    MainWindow mainWindow;
    // Prevents a warning on Windows.
    mainWindow.setGeometry( 200, 200, 200, 200 );
    mainWindow.showMaximized();
    app.installEventFilter( (QObject*)mainWindow.GetSceneWidget() );

    return app.exec();
}

