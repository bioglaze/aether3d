#include <QMenu>
#include <QMenuBar>
#include "WindowMenu.hpp"

void WindowMenu::Init( QWidget* mainWindow )
{
    connect(mainWindow, SIGNAL(GameObjectSelected(std::list< ae3d::GameObject* >)),
            this, SLOT(GameObjectSelected(std::list< ae3d::GameObject* >)));
    menuBar = new QMenuBar( mainWindow );

    fileMenu = menuBar->addMenu( "&File" );
    fileMenu->addAction( "Save Scene", mainWindow, SLOT(SaveScene()), QKeySequence("Ctrl+S"));
    fileMenu->addAction( "Open Scene", mainWindow, SLOT(LoadScene()), QKeySequence("Ctrl+O"));
    fileMenu->addAction( "About", mainWindow, SLOT(ShowAbout()), QKeySequence("Ctrl+A"));
    //fileMenu->addAction( "Exit", mainWindow, SLOT(ShowAbout()), QKeySequence("Ctrl+Q"));

    //fileMenu->addSeparator();

    editMenu = menuBar->addMenu( "&Edit" );
    undo = editMenu->addAction( "Undo", mainWindow, SLOT(Undo()), QKeySequence("Ctrl+Z"));

    sceneMenu = menuBar->addMenu( "&Scene" );
    sceneMenu->addAction( "Lighting", mainWindow, SLOT(OpenLightingInspector()), QKeySequence("Ctrl+L"));
    sceneMenu->addAction( "Create Game Object", mainWindow, SLOT(CommandCreateGameObject()), QKeySequence("Ctrl+N"));

    componentMenu = menuBar->addMenu( "&Component" );
    componentMenu->addAction( "Add Audio Source", mainWindow, SLOT(CommandCreateAudioSourceComponent()));
    componentMenu->addAction( "Add Camera", mainWindow, SLOT(CommandCreateCameraComponent()));
    componentMenu->addAction( "Add Directional Light", mainWindow, SLOT(CommandCreateDirectionalLightComponent()));
    componentMenu->addAction( "Add Mesh Renderer", mainWindow, SLOT(CommandCreateMeshRendererComponent()));
    componentMenu->addAction( "Add Sprite Renderer", mainWindow, SLOT(CommandCreateSpriteRendererComponent()));
    componentMenu->addAction( "Add Spot Light", mainWindow, SLOT(CommandCreateSpotLightComponent()));
    componentMenu->addAction( "Add Point Light", mainWindow, SLOT(CommandCreatePointLightComponent()));
}

void WindowMenu::GameObjectSelected( std::list< ae3d::GameObject* > gameObjects )
{
    componentMenu->setEnabled( !gameObjects.empty() );
}

void WindowMenu::SetUndoText( const char* text )
{
    undo->setText( text );
}
