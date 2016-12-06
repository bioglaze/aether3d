#include <QApplication>
#include "MainWindow.hpp"
#include "System.hpp"

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

