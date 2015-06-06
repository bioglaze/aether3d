#include <QSplitter>
#include <QtOpenGL/QGLWidget>
#include "MainWindow.hpp"
#include "SceneWidget.hpp"
#include <iostream>

MainWindow::MainWindow()
{
    QGLFormat gFormat;
    gFormat.setVersion( 4, 1 );
    gFormat.setSwapInterval( 1 );
    gFormat.setProfile( QGLFormat::CoreProfile );

    sceneWidget = new SceneWidget(gFormat);
    setWindowTitle( tr( "Editor" ) );

    QSplitter* splitter = new QSplitter();
    splitter->addWidget( sceneWidget );
    setCentralWidget( splitter );
    std::cout << "mutsis" << std::endl;
}


