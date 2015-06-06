#include <QApplication>
#include "MainWindow.hpp"

int main( int argc, char *argv[] )
{
    QApplication a( argc, argv );
    a.setKeyboardInputInterval( 10 );

    MainWindow mainWindow;
    mainWindow.showMaximized();
    a.installEventFilter( (QObject*)mainWindow.GetSceneWidget() );

    return a.exec();
}

