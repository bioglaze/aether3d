#include <QSplitter>
#include <QTreeWidget>
#include "MainWindow.hpp"
#include "SceneWidget.hpp"
#include "WindowMenu.hpp"
#include "CreateCameraCommand.hpp"
#include "CreateGoCommand.hpp"
#include <iostream>
#include <memory>

MainWindow::MainWindow()
{
    sceneTree = new QTreeWidget();
    sceneTree->setColumnCount( 1 );
    connect( sceneTree, &QTreeWidget::itemClicked, [&](QTreeWidgetItem* item, int /* column */) { std::cout << "jee" << std::endl;/*SelectTreeItem( item );*/ });

    sceneWidget = new SceneWidget();
    setWindowTitle( tr( "Editor" ) );

    windowMenu.Init( this );
    setMenuBar( windowMenu.menuBar );

    QSplitter* splitter = new QSplitter();
    splitter->addWidget( sceneTree );
    splitter->addWidget( sceneWidget );
    setCentralWidget( splitter );

    UpdateHierarchy();
}

void MainWindow::CommandCreateCamera()
{
    std::cout << "MainWindow CreateCamera" << std::endl;
    commandManager.Execute( std::make_shared< CreateCameraCommand >() );
}

void MainWindow::CommandCreateGameObject()
{
    std::cout << "MainWindow CommandCreateGameObject" << std::endl;
    commandManager.Execute( std::make_shared< CreateGoCommand >( sceneWidget ) );
    UpdateHierarchy();
}

void MainWindow::LoadScene()
{
    std::cout << "MainWindow loadscene" << std::endl;
}

void MainWindow::UpdateHierarchy()
{
    sceneTree->clear();
    sceneTree->setHeaderLabel( "Hierarchy" );

    QList< QTreeWidgetItem* > nodes;
    int count = sceneWidget->GetGameObjectCount();

    for (std::size_t i = 0; i < count; ++i)
    {
        nodes.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString(sceneWidget->GetGameObject(i)->GetName().c_str()))));
        //m->treeWidgetToNode[ nodes.back() ] = node;

        /*if (node == editorState->selectedNode)
        {
            sceneTree->setCurrentItem( nodes.back() );
        }*/
    }
    /*for (auto& node : sceneWidget->GetScene()->Children())
    {
        if (!sceneWidget->IsEditorWidget( node ))
        {
            nodes.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString(node->GetName().c_str()))));
            m->treeWidgetToNode[ nodes.back() ] = node;

            if (node == editorState->selectedNode)
            {
                sceneTree->setCurrentItem( nodes.back() );
            }
        }
    }*/

    sceneTree->insertTopLevelItems( 0, nodes );
}
