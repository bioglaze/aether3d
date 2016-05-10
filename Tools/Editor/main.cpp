#include <QApplication>
#include "MainWindow.hpp"
#include "System.hpp"

// FIXME: deleting gameobjects causes memory corruption that does not crash instantly.
// FIXME: sometimes transform gizmo disappears or is misplaced
// FIXME: Gizmo is obscured by model
// TODO: inspector vertical scroll bar
// TODO: game object delete command
// TODO: game object rename command
// TODO: Ray-Triangle intersection for selection.
// TODO: Duplicate entire selection
/*qDebug( "debug" );
qInfo( "info" );
qCritical( "critical" );
qFatal( "fatal" );
qWarning( "average position: %f, %f, %f", SelectionAveragePosition().x, SelectionAveragePosition().y, SelectionAveragePosition().z );
*/

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

