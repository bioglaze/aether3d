#include "AudioSourceInspector.hpp"
#include <string>
#include <QBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWidget>
#include <QPushButton>
#include "AudioSourceComponent.hpp"
#include "AudioClip.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"

void AudioSourceInspector::Init( QWidget* mainWindow )
{
    audioClipTable = new QTableWidget( 1, 1 );
    audioClipTable->setItem( 0, 0, new QTableWidgetItem() );
    audioClipTable->setHorizontalHeaderLabels( QString("").split(";") );
    audioClipTable->setVerticalHeaderLabels( QString("Filename").split(";") );

    QLabel* componentName = new QLabel("Audio Source");
    removeButton = new QPushButton("remove");
    QBoxLayout* headerLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    headerLayout->addWidget( componentName );
    headerLayout->addWidget( removeButton );
    QWidget* header = new QWidget();
    header->setLayout( headerLayout );

    QBoxLayout* inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->setContentsMargins( 1, 1, 1, 1 );
    inspectorLayout->addWidget( header );
    inspectorLayout->addWidget( audioClipTable );

    root = new QWidget();
    root->setLayout( inspectorLayout );

    connect( removeButton, SIGNAL(clicked(bool)), mainWindow, SLOT(CommandRemoveAudioSourceComponent()));
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
    const std::string path = QFileDialog::getOpenFileName( root, "Open Audio Clip", "", "Audio clips (*.wav *.ogg)" ).toStdString();

    if (!path.empty())
    {
        ae3d::AudioClip* audioClip = new ae3d::AudioClip();
        audioClip->Load( ae3d::FileSystem::FileContents( path.c_str() ) );
        audioClipTable->setItem( 0, 0, new QTableWidgetItem( path.c_str() ) );
    }
}
