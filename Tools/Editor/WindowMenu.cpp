#include <QMenu>
#include <QMenuBar>
#include "WindowMenu.hpp"

void WindowMenu::Init( QWidget* mainWindow )
{
    menuBar = new QMenuBar( mainWindow );

    fileMenu = menuBar->addMenu( "&File" );
    fileMenu->addAction( "Load Scene", mainWindow, SLOT(LoadScene()), QKeySequence("Ctrl+O"));
    //fileMenu->addSeparator();

    editMenu = menuBar->addMenu( "&Edit" );
    editMenu->addAction( "Undo", mainWindow, SLOT(Undo()), QKeySequence("Ctrl+Z"));

    sceneMenu = menuBar->addMenu( "&Scene" );
    sceneMenu->addAction( "Create Game Object", mainWindow, SLOT(CommandCreateGameObject()), QKeySequence("Ctrl+N"));

    componentMenu = menuBar->addMenu( "&Component" );
    componentMenu->addAction( "Add Camera", mainWindow, SLOT(CommandCreateCameraComponent()));
}
