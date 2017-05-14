#include <QMenu>
#include <QMenuBar>
#include "WindowMenu.hpp"
#include "GameObject.hpp"
#include "AudioSourceComponent.hpp"
#include "CameraComponent.hpp"
#include "DirectionalLightComponent.hpp"
#include "MeshRendererComponent.hpp"
#include "SpotLightComponent.hpp"
#include "PointLightComponent.hpp"
#include "SpriteRendererComponent.hpp"

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
    actionAddAudioSource = componentMenu->addAction( "Add Audio Source", mainWindow, SLOT(CommandCreateAudioSourceComponent()));
    actionAddCamera = componentMenu->addAction( "Add Camera", mainWindow, SLOT(CommandCreateCameraComponent()));
    actionAddDirLight = componentMenu->addAction( "Add Directional Light", mainWindow, SLOT(CommandCreateDirectionalLightComponent()));
    actionAddMesh = componentMenu->addAction( "Add Mesh Renderer", mainWindow, SLOT(CommandCreateMeshRendererComponent()));
    actionAddSpriteRenderer = componentMenu->addAction( "Add Sprite Renderer", mainWindow, SLOT(CommandCreateSpriteRendererComponent()));
    actionAddSpotLight = componentMenu->addAction( "Add Spot Light", mainWindow, SLOT(CommandCreateSpotLightComponent()));
    actionAddPointLight = componentMenu->addAction( "Add Point Light", mainWindow, SLOT(CommandCreatePointLightComponent()));
}

void WindowMenu::Update()
{
    componentMenu->setEnabled( !gameObjects.empty() );

    if (!gameObjects.empty())
    {
        actionAddAudioSource->setEnabled( !gameObjects.front()->GetComponent< ae3d::AudioSourceComponent >() );
        actionAddCamera->setEnabled( !gameObjects.front()->GetComponent< ae3d::CameraComponent >() );
        actionAddDirLight->setEnabled( !gameObjects.front()->GetComponent< ae3d::DirectionalLightComponent >() );
        actionAddSpotLight->setEnabled( !gameObjects.front()->GetComponent< ae3d::SpotLightComponent >() );
        actionAddPointLight->setEnabled( !gameObjects.front()->GetComponent< ae3d::PointLightComponent >() );
        actionAddMesh->setEnabled( !gameObjects.front()->GetComponent< ae3d::MeshRendererComponent >() );
        actionAddSpriteRenderer->setEnabled( !gameObjects.front()->GetComponent< ae3d::SpriteRendererComponent >() );
    }
}

void WindowMenu::GameObjectSelected( std::list< ae3d::GameObject* > selectedGameObjects )
{
    gameObjects = selectedGameObjects;
    Update();
}

void WindowMenu::SetUndoText( const char* text )
{
    undo->setText( text );
}
