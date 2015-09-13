#include "MeshRendererInspector.hpp"
#include <QBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QFileDialog>
#include <QMainWindow>
#include "GameObject.hpp"
#include "MeshRendererComponent.hpp"

void MeshRendererInspector::Init( QWidget* aMainWindow )
{
    mainWindow = aMainWindow;

    // TODO: change type into something more intelligible.
    meshTable = new QTableWidget( 1, 1 );
    meshTable->setItem( 0, 0, new QTableWidgetItem() );
    //meshTable->setHorizontalHeaderLabels( QString("Mesh").split(";") );
    meshTable->setVerticalHeaderLabels( QString("Mesh").split(";") );

    QLabel* componentName = new QLabel("Mesh Renderer");

    QBoxLayout* inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->setContentsMargins( 1, 1, 1, 1 );
    inspectorLayout->addWidget( componentName );
    inspectorLayout->addWidget( meshTable );

    root = new QWidget();
    root->setLayout( inspectorLayout );

    connect(meshTable, SIGNAL(cellClicked(int,int)), this, SLOT(MeshCellClicked(int,int)));
}

void MeshRendererInspector::MeshCellClicked( int, int )
{
    //QMainWindow* qMainWindow = static_cast< QMainWindow* >( mainWindow );
    const std::string path = QFileDialog::getOpenFileName( root/*qMainWindow->centralWidget()*/, "Open Mesh", "", "Meshes (*.ae3d)" ).toStdString();

    if (!path.empty())
    {

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

    auto meshRendererComponent = gameObject->GetComponent< ae3d::MeshRendererComponent >();

    if (meshRendererComponent)
    {

    }
}
