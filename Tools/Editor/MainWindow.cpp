#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include <QSplitter>
#include <QTreeWidget>
#include <QKeyEvent>
#include <QFileDialog>
#include <QMessageBox>
#include "MainWindow.hpp"
#include "SceneWidget.hpp"
#include "WindowMenu.hpp"
#include "CreateCameraCommand.hpp"
#include "CreateGoCommand.hpp"

MainWindow::MainWindow()
{
    sceneTree = new QTreeWidget();
    sceneTree->setColumnCount( 1 );
    connect( sceneTree, &QTreeWidget::itemClicked, [&](QTreeWidgetItem* item, int /* column */) { SelectTreeItem( item ); });

    sceneWidget = new SceneWidget();
    setWindowTitle( "Editor" );

    windowMenu.Init( this );
    setMenuBar( windowMenu.menuBar );

    QSplitter* splitter = new QSplitter();
    splitter->addWidget( sceneTree );
    splitter->addWidget( sceneWidget );
    setCentralWidget( splitter );

    UpdateHierarchy();
}

void MainWindow::SelectTreeItem( QTreeWidgetItem* item )
{
    //SetSelectedNode( m->treeWidgetToNode[ item ] );
    int g = 0;

    for (int i = 0; i < sceneWidget->GetGameObjectCount(); ++i)
    {
        if (sceneWidget->IsGameObjectInScene( i ))
        {
            //if (sceneTree->ite)
            ++g;
        }
    }
}

void MainWindow::keyReleaseEvent( QKeyEvent* event )
{
}

void MainWindow::CommandCreateCameraComponent()
{
    commandManager.Execute( std::make_shared< CreateCameraCommand >( sceneWidget ) );
}

void MainWindow::CommandCreateGameObject()
{
    commandManager.Execute( std::make_shared< CreateGoCommand >( sceneWidget ) );
    UpdateHierarchy();
}

void MainWindow::LoadScene()
{
    const QString fileName = QFileDialog::getOpenFileName( this, "Open Scene", "", "Scenes (*.scene)" );

    if (fileName != "")
    {
    }
}

void MainWindow::SaveScene()
{
#if __APPLE__
    const char* appendDir = "/Documents";
#else
    const char* appendDir = "";
#endif

    QString fileName = QFileDialog::getSaveFileName( this, "Save Scene", QDir::homePath() + appendDir, "Scenes (*.scene)" );
    std::ofstream ofs( fileName.toUtf8().constData() );
    ofs << sceneWidget->GetScene()->GetSerialized();
    const bool saveSuccess = ofs.is_open();

    if (!saveSuccess)
    {
        QMessageBox::critical(this, "Unable to Save!", "Scene could not be saved.");
    }
}

void MainWindow::UpdateHierarchy()
{
    sceneTree->clear();
    sceneTree->setHeaderLabel( "Hierarchy" );

    QList< QTreeWidgetItem* > nodes;
    const int count = sceneWidget->GetGameObjectCount();
    int g = 0;

    for (int i = 0; i < count; ++i)
    {
        if (sceneWidget->IsGameObjectInScene( i ))
        {
            nodes.append(new QTreeWidgetItem( (QTreeWidget*)0, QStringList( QString( sceneWidget->GetGameObject( g )->GetName().c_str() ) ) ) );

            ++g;
        }
    }

    sceneTree->insertTopLevelItems( 0, nodes );

    // Highlights selected game object.
    for (int i = 0; i < g; ++i)
    {
        if (i == sceneWidget->selectedGameObject)
        {
            sceneTree->setCurrentItem( nodes.at(i) );
        }
    }
}
