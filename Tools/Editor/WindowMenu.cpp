#include <QMenu>
#include <QMenuBar>
#include "WindowMenu.hpp"
#include <iostream>

void WindowMenu::Init( QWidget* mainWindow )
{
    connect(mainWindow, SIGNAL(GameObjectSelected(ae3d::GameObject*)), this, SLOT(GameObjectSelected(ae3d::GameObject*)));
    menuBar = new QMenuBar( mainWindow );

    fileMenu = menuBar->addMenu( "&File" );
    fileMenu->addAction( "Save Scene", mainWindow, SLOT(SaveScene()), QKeySequence("Ctrl+S"));
    //fileMenu->addSeparator();

    editMenu = menuBar->addMenu( "&Edit" );
    editMenu->addAction( "Undo", mainWindow, SLOT(Undo()), QKeySequence("Ctrl+Z"));

    sceneMenu = menuBar->addMenu( "&Scene" );
    sceneMenu->addAction( "Create Game Object", mainWindow, SLOT(CommandCreateGameObject()), QKeySequence("Ctrl+N"));

    componentMenu = menuBar->addMenu( "&Component" );
    componentMenu->addAction( "Add Camera", mainWindow, SLOT(CommandCreateCameraComponent()));
}

void WindowMenu::GameObjectSelected( ae3d::GameObject* gameObject )
{
    componentMenu->setEnabled( gameObject != nullptr );
}
