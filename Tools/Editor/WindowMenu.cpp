#include <QMenu>
#include <QMenuBar>
#include "WindowMenu.hpp"
#include <iostream>

WindowMenu::WindowMenu()
{
}

WindowMenu::~WindowMenu()
{

}

/*void test(QAction* action)
{
    std::cout << "action: " << action->text().toStdString() << std::endl;
}*/

void WindowMenu::Init( QWidget* mainWindow )
{
    createCameraAction = new QAction( mainWindow );
    createCameraAction->setText( tr("Create Camera") );
    connect( createCameraAction, SIGNAL(triggered()), mainWindow, SLOT(CommandCreateCamera()) );

    loadSceneAction = new QAction( mainWindow );
    loadSceneAction->setText( "Load Scene" );
    // TODO: Figure out how to do this with the new typesafe connection system.
    //connect( loadSceneAction, SIGNAL(triggered()), this, SLOT(LoadScene()) );
    connect( loadSceneAction, SIGNAL(triggered()), mainWindow, SLOT(LoadScene()) );

    createGoAction = new QAction( mainWindow );
    createGoAction->setText( "Create Game Object" );
    // TODO: Figure out how to do this with the new typesafe connection system.
    //connect( loadSceneAction, SIGNAL(triggered()), this, SLOT(LoadScene()) );
    connect( createGoAction, SIGNAL(triggered()), mainWindow, SLOT(CommandCreateGameObject()) );

    menuBar = new QMenuBar( mainWindow );
    fileMenu = menuBar->addMenu( "&File" );
    fileMenu->addAction( loadSceneAction );
    //connect( fileMenu, &QMenu::triggered, this, [&](QAction* action) { std::cout << "open action" << std::endl; } );
    //connect( fileMenu, &QMenu::triggered, this, &freefunction );

    sceneMenu = menuBar->addMenu( "&Scene" );
    sceneMenu->addAction( createGoAction );

}
