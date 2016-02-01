#include "AudioSourceInspector.hpp"
#include <string>
#include <QBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWidget>
#include "AudioSourceComponent.hpp"
#include "AudioClip.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"

void AudioSourceInspector::Init( QWidget* mainWindow )
{
    auto componentName = new QLabel( "Audio Source" );

    audioClipTable = new QTableWidget( 1, 1 );
    audioClipTable->setItem( 0, 0, new QTableWidgetItem() );
    audioClipTable->setHorizontalHeaderLabels( QString("").split(";") );
    audioClipTable->setVerticalHeaderLabels( QString("Filename").split(";") );

    audioClipTable->setItem( 0, 0, new QTableWidgetItem() );

    QBoxLayout* inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->setContentsMargins( 1, 1, 1, 1 );
    inspectorLayout->addWidget( componentName );
    inspectorLayout->addWidget( audioClipTable );

    root = new QWidget();
    root->setLayout( inspectorLayout );

    connect( audioClipTable, SIGNAL(cellClicked(int,int)), this, SLOT(AudioCellClicked(int, int)) );
    connect( mainWindow, SIGNAL(GameObjectSelected(std::list< ae3d::GameObject* >)),
             this, SLOT(GameObjectSelected(std::list< ae3d::GameObject* >)) );
}

void AudioSourceInspector::GameObjectSelected( std::list< ae3d::GameObject* > gameObjects )
{
    if (gameObjects.empty() || !gameObjects.front()->GetComponent< ae3d::AudioSourceComponent >())
    {
        return;
    }
}

void AudioSourceInspector::AudioCellClicked( int, int )
{
    // TOOD: Asset library, this is only an ugly placeholder.

    const std::string path = QFileDialog::getOpenFileName( root, "Open Audio Clip", "", "Audio clips (*.wav)" ).toStdString();

    if (!path.empty())
    {
        ae3d::AudioClip* audioClip = new ae3d::AudioClip();
        audioClip->Load( ae3d::FileSystem::FileContents( path.c_str() ) );
    }
}
