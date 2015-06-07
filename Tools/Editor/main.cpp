#include <QApplication>
#include "MainWindow.hpp"

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

