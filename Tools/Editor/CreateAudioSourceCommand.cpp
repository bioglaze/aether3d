#include "CreateAudioSourceCommand.hpp"
#include "AudioSourceComponent.hpp"
#include "GameObject.hpp"
#include "System.hpp"
#include "SceneWidget.hpp"

CreateAudioSourceCommand::CreateAudioSourceCommand( SceneWidget* aSceneWidget )
{
    sceneWidget = aSceneWidget;
}

void CreateAudioSourceCommand::Execute()
{
    ae3d::System::Assert( sceneWidget != nullptr, "Create camera needs scene." );
    ae3d::System::Assert( !sceneWidget->selectedGameObjects.empty(), "no gameobjects selected." );

    gameObject = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() );

    if (!gameObject->GetComponent< ae3d::AudioSourceComponent >())
    {
        gameObject->AddComponent< ae3d::AudioSourceComponent >();
    }
}

void CreateAudioSourceCommand::Undo()
{
    gameObject->RemoveComponent< ae3d::AudioSourceComponent >();
}
