#include "MeshRendererInspector.hpp"
#include <QBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QFileDialog>
#include <QMainWindow>
#include "GameObject.hpp"
#include "FileSystem.hpp"
#include "System.hpp"
#include "MeshRendererComponent.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

extern ae3d::Material* gCubeMaterial;

void MeshRendererInspector::Init( QWidget* aMainWindow )
{
    mainWindow = aMainWindow;

    // TODO: change type into something more intelligible.
    meshTable = new QTableWidget( 1, 1 );
    meshTable->setItem( 0, 0, new QTableWidgetItem() );
    meshTable->setHorizontalHeaderLabels( QString("").split(";") );
    meshTable->setVerticalHeaderLabels( QString("Mesh").split(";") );

    QLabel* componentName = new QLabel("Mesh Renderer");

    QBoxLayout* inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->setContentsMargins( 1, 1, 1, 1 );
    inspectorLayout->addWidget( componentName );
    inspectorLayout->addWidget( meshTable );

    root = new QWidget();
    root->setLayout( inspectorLayout );

    connect( meshTable, SIGNAL(cellClicked(int,int)), this, SLOT(MeshCellClicked(int,int)) );
    connect( mainWindow, SIGNAL(GameObjectSelected(std::list< ae3d::GameObject* >)),
             this, SLOT(GameObjectSelected(std::list< ae3d::GameObject* >)) );
}

void MeshRendererInspector::MeshCellClicked( int, int )
{
    // TOOD: Asset library, this is only an ugly placeholder.

    //QMainWindow* qMainWindow = static_cast< QMainWindow* >( mainWindow );
    const std::string path = QFileDialog::getOpenFileName( root/*qMainWindow->centralWidget()*/, "Open Mesh", "", "Meshes (*.ae3d)" ).toStdString();

    if (!path.empty() && gameObject)
    {
        ae3d::Mesh* mesh = new ae3d::Mesh();
        ae3d::Mesh::LoadResult result = mesh->Load( ae3d::FileSystem::FileContents( path.c_str() ) );

        if (result == ae3d::Mesh::LoadResult::Success)
        {
            auto meshRendererComponent = gameObject->GetComponent< ae3d::MeshRendererComponent >();
            meshRendererComponent->SetMesh( mesh );

            for (int i = 0, length = meshRendererComponent->GetMesh()->GetSubMeshCount(); i < length; ++i)
            {
                meshRendererComponent->SetMaterial( gCubeMaterial, i );
            }
        }
        else
        {
            ae3d::System::Print( "Unable to load mesh %s\n", path.c_str() );
            delete mesh;
        }
    }
}

void MeshRendererInspector::GameObjectSelected( std::list< ae3d::GameObject* > gameObjects )
{
    if (gameObjects.empty())
    {
        gameObject = nullptr;
    }
    else
    {
        gameObject = gameObjects.front();
    }
}
