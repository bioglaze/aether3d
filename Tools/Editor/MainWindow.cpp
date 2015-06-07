#include <QSplitter>
#include <QTreeWidget>
#include "MainWindow.hpp"
#include "SceneWidget.hpp"
#include "WindowMenu.hpp"
#include <iostream>

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

void MainWindow::UpdateHierarchy()
{
    sceneTree->clear();
    sceneTree->setHeaderLabel( "Hierarchy" );
}
