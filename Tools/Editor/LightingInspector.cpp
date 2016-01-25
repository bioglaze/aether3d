#include "LightingInspector.hpp"
#include <QLabel>
#include <QBoxLayout>
#include <QCheckBox>
#include <QFileDialog>
#include <QTableWidget>
#include "FileSystem.hpp"
#include "SceneWidget.hpp"

void LightingInspector::Init( SceneWidget* aSceneWidget )
{
    auto componentName = new QLabel( "Scene Lighting" );
    auto skyboxLabel = new QLabel( "Skybox" );

    skyboxTable = new QTableWidget( 6, 1 );
    skyboxTable->setItem( 0, 0, new QTableWidgetItem() );
    skyboxTable->setHorizontalHeaderLabels( QString("Texture").split(";") );
    skyboxTable->setVerticalHeaderLabels( QString("negX;posX;negY;posY;negZ;posZ").split(";") );

    for (int r = 0; r < 6; ++r)
    {
        skyboxTable->setItem( r, 0, new QTableWidgetItem() );
    }

    auto inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->setContentsMargins( 1, 1, 1, 1 );
    inspectorLayout->addWidget( componentName );
    inspectorLayout->addWidget( skyboxLabel );
    inspectorLayout->addWidget( skyboxTable );

    sceneWidget = aSceneWidget;

    root = new QWidget();
    root->setLayout( inspectorLayout );

    connect( skyboxTable, SIGNAL(cellClicked(int,int)), this, SLOT(SkyboxCellClicked(int,int)) );
}

void LightingInspector::SkyboxCellClicked( int row, int col )
{
    // TODO: Asset library
    const std::string path = QFileDialog::getOpenFileName( root, "Open Texture 2D", "", "" ).toStdString();

    if (!path.empty())
    {
        skyboxTexturePaths[ row ] = path;
        skyboxTable->item( row, col )->setText( path.c_str() );
    }

    skyboxCube.Load( ae3d::FileSystem::FileContents( skyboxTexturePaths[ 0 ].c_str() ), ae3d::FileSystem::FileContents( skyboxTexturePaths[ 1 ].c_str() ),
                     ae3d::FileSystem::FileContents( skyboxTexturePaths[ 2 ].c_str() ), ae3d::FileSystem::FileContents( skyboxTexturePaths[ 3 ].c_str() ),
                     ae3d::FileSystem::FileContents( skyboxTexturePaths[ 4 ].c_str() ), ae3d::FileSystem::FileContents( skyboxTexturePaths[ 5 ].c_str() ),
                     ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Nearest,
                     ae3d::Mipmaps::None, ae3d::ColorSpace::SRGB );
    //sceneWidget->SetSkybox( skyboxCube );
}
