#include "MeshRendererInspector.hpp"
#include <vector>
#include <map>
#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QFileDialog>
#include <QMainWindow>
#include "GameObject.hpp"
#include "FileSystem.hpp"
#include "SceneWidget.hpp"
#include "System.hpp"
#include "MeshRendererComponent.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "GameObject.hpp"

using namespace ae3d;

extern ae3d::Material* gCubeMaterial;

void MeshRendererInspector::Init( QWidget* aMainWindow, SceneWidget* aSceneWidget )
{
    mainWindow = aMainWindow;
    sceneWidget = aSceneWidget;

    // TODO: change type into something more intelligible.
    meshTable = new QTableWidget( 1, 2 );
    meshTable->setItem( 0, 0, new QTableWidgetItem() );
    meshTable->setHorizontalHeaderLabels( QString("Mesh;Material").split(";") );

    QLabel* componentName = new QLabel("Mesh Renderer");
    removeButton = new QPushButton("remove");
    QBoxLayout* headerLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    headerLayout->addWidget( componentName );
    headerLayout->addWidget( removeButton );
    QWidget* header = new QWidget();
    header->setLayout( headerLayout );

    QBoxLayout* inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->setContentsMargins( 1, 1, 1, 1 );
    inspectorLayout->addWidget( header );
    inspectorLayout->addWidget( meshTable );

    root = new QWidget();
    root->setLayout( inspectorLayout );

    connect( meshTable, SIGNAL(cellClicked(int,int)), this, SLOT(MeshCellClicked(int,int)) );
    connect( mainWindow, SIGNAL(GameObjectSelected(std::list< ae3d::GameObject* >)),
             this, SLOT(GameObjectSelected(std::list< ae3d::GameObject* >)) );
    connect( removeButton, SIGNAL(clicked(bool)), mainWindow, SLOT(CommandRemoveMeshRendererComponent()));
}

void MeshRendererInspector::MeshCellClicked( int row, int col )
{
    // TOOD: Asset library, this is only an ugly placeholder.

    // Material
    if (col == 1)
    {
        const std::string path = QFileDialog::getOpenFileName( root, "Open Material", "", "Materials (*.material)" ).toStdString();

        if (!path.empty() && gameObject)
        {
            std::vector< GameObject > gameObjects;
            std::map< std::string, Material* > materialNameToMaterial;
            std::map< std::string, Texture2D* > textureNameToTexture;
            std::vector< Mesh* > meshes;

            auto res = sceneWidget->GetScene()->Deserialize( FileSystem::FileContents( path.c_str() ), gameObjects, textureNameToTexture,
                                         materialNameToMaterial, meshes );

            if (res != Scene::DeserializeResult::Success)
            {
                System::Print( "Could not parse %s\n", path.c_str() );
            }

            if (!materialNameToMaterial.empty())
            {
                ae3d::System::Print("loaded material\n");
                auto meshRendererComponent = gameObject->GetComponent< ae3d::MeshRendererComponent >();
                meshRendererComponent->SetMaterial( materialNameToMaterial.begin()->second, row );
            }
            else
            {
                ae3d::System::Print( "Could not find material definition in %s\n", path.c_str() );
            }
            //ae3d::FileSystem::FileContentsData contents = ae3d::FileSystem::FileContents( path.c_str() );
            //std::string contentsStr( contents.data.begin(), contents.data.end() );
           // material->Deserialize(  );
        }

        return;
    }

    const std::string path = QFileDialog::getOpenFileName( root, "Open Mesh", "", "Meshes (*.ae3d)" ).toStdString();

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
