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
    // TODO: Change itemClicked to something that can handle multi-selection properly.
    connect( sceneTree, &QTreeWidget::itemClicked, [&](QTreeWidgetItem* item, int /* column */) { SelectTreeItem( item ); });
    sceneTree->setSelectionMode( QAbstractItemView::SelectionMode::ExtendedSelection );

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
    int sceneObjectCounter = 0;

    for (int i = 0; i < sceneWidget->GetGameObjectCount(); ++i)
    {
        if (sceneWidget->IsGameObjectInScene( i ))
        {
            ++sceneObjectCounter;
        }
    }

    int selectedIndex = 0;

    // Figures out which game object was selected.
    for (int i = 0; i < sceneObjectCounter; ++i)
    {
        if (sceneTree->topLevelItem( i ) == item)
        {
            selectedIndex = i;
        }
    }
    std::cout << "printing selection: " << std::endl;
for (auto index : sceneWidget->selectedGameObjects)
{
std::cout << "selection: " << index << std::endl;
}
    // TODO: Remove sceneWidget->selectedGameObject variable.
    sceneWidget->selectedGameObjects.clear();
    sceneWidget->selectedGameObjects.push_back( selectedIndex/*sceneWidget->GetGameObject( selectedIndex )*/ );
    std::list< ae3d::GameObject* > selectedObjects;
    selectedObjects.push_back( sceneWidget->GetGameObject( selectedIndex ) );
    emit GameObjectSelected( selectedObjects );
}

void MainWindow::keyPressEvent( QKeyEvent* event )
{
    const int macDelete = 16777219;

    if (event->key() == Qt::Key_Escape)
    {
        std::list< ae3d::GameObject* > emptyList;
        emit GameObjectSelected( emptyList );
    }
    else if (event->key() == Qt::Key_Delete || event->key() == macDelete)
    {
        for (auto goIndex : sceneWidget->selectedGameObjects)
        {
            sceneWidget->RemoveGameObject( goIndex );
            std::cout << "removed " << goIndex << std::endl;
        }

        UpdateHierarchy();
    }
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

    // Highlights selected game objects.
    for (int i = 0; i < g; ++i)
    {
        for (auto goIndex : sceneWidget->selectedGameObjects)
        {
            if (i == goIndex)
            {
                sceneTree->setCurrentItem( nodes.at(i) );
            }
        }
    }
}
