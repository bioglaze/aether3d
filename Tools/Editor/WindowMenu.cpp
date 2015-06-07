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

void WindowMenu::LoadScene()
{
    std::cout << "Load Scene" << std::endl;
}

/*void test(QAction* action)
{
    std::cout << "action: " << action->text().toStdString() << std::endl;
}*/

void WindowMenu::Init( QWidget* mainWindow )
{
    loadSceneAction = new QAction( mainWindow );
    loadSceneAction->setText( "Load Scene" );
    // TODO: Figure out how to do this with the new connection system.
    connect( loadSceneAction, SIGNAL(triggered()), this, SLOT(LoadScene()) );

    menuBar = new QMenuBar( mainWindow );
    fileMenu = menuBar->addMenu( "&File" );
    fileMenu->addAction( loadSceneAction );
    //connect( fileMenu, &QMenu::triggered, this, [&](QAction* action) { std::cout << "open action" << std::endl; } );
    //connect( fileMenu, &QMenu::triggered, this, &freefunction );
}
