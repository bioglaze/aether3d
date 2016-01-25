#include <QApplication>
#include "MainWindow.hpp"
#include "System.hpp"

// FIXME: deleting gameobjects causes memory corruption that does not crash instantly.
// FIXME: sometimes transform gizmo disappears or is misplaced
// TODO: game object delete command
// TODO: game object rename command
// TODO: Ray-Triangle intersection for selection.
// TODO: Prevent empty name for game object
// TODO: Duplicate entire selection
// TODO: Remove hard-coded startup scene contents and load them from a startup scene.

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

