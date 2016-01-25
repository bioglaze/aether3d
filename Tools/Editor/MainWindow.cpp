#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include <QBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QKeyEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QWindow>
#include "CameraComponent.hpp"
#include "DirectionalLightComponent.hpp"
#include "SpotLightComponent.hpp"
#include "MainWindow.hpp"
#include "SceneWidget.hpp"
#include "System.hpp"
#include "WindowMenu.hpp"
#include "CreateCameraCommand.hpp"
#include "CreateGoCommand.hpp"
#include "CreateLightCommand.hpp"
#include "CreateMeshRendererCommand.hpp"
#include "MeshRendererComponent.hpp"
#include "ModifyTransformCommand.hpp"
#include "ModifyCameraCommand.hpp"
#include "TransformInspector.hpp"
#include "Quaternion.hpp"

MainWindow::MainWindow()
{
    sceneTree = new QTreeWidget();
    sceneTree->setColumnCount( 1 );
    connect( sceneTree, &QTreeWidget::itemSelectionChanged, [&]() { HierarchySelectionChanged(); });
    connect( sceneTree, &QTreeWidget::itemChanged, [&](QTreeWidgetItem* item, int /* column */) { HierarchyItemRenamed( item ); });
    sceneTree->setSelectionMode( QAbstractItemView::SelectionMode::ExtendedSelection );

    sceneWidget = new SceneWidget();
    sceneWidget->SetMainWindow( this );
    setWindowTitle( "Editor" );

    connect( sceneWidget, &SceneWidget::GameObjectsAddedOrDeleted, this, &MainWindow::HandleGameObjectsAddedOrDeleted );
    connect( &transformInspector, &TransformInspector::TransformModified, this, &MainWindow::CommandModifyTransform );
    connect( sceneWidget, &SceneWidget::TransformModified, this, &MainWindow::CommandModifyTransform );
    connect( this, &MainWindow::GameObjectSelected, this, &MainWindow::OnGameObjectSelected );
    connect( &cameraInspector, &CameraInspector::CameraModified, this, &MainWindow::CommandModifyCamera );

    windowMenu.Init( this );
    setMenuBar( windowMenu.menuBar );

    transformInspector.Init( this );
    cameraInspector.Init( this );
    meshRendererInspector.Init( this );
    dirLightInspector.Init( this );
    spotLightInspector.Init( this );
    lightingInspector.Init( sceneWidget );

    QBoxLayout* inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->addWidget( transformInspector.GetWidget() );
    inspectorLayout->addWidget( cameraInspector.GetWidget() );
    inspectorLayout->addWidget( meshRendererInspector.GetWidget() );
    inspectorLayout->addWidget( dirLightInspector.GetWidget() );
    inspectorLayout->addWidget( spotLightInspector.GetWidget() );
    inspectorLayout->addWidget( lightingInspector.GetWidget() );

    inspectorContainer = new QWidget();
    inspectorContainer->setLayout( inspectorLayout );
    inspectorContainer->setMinimumWidth( 380 );

    QSplitter* splitter = new QSplitter();
    splitter->addWidget( sceneTree );
    splitter->addWidget( sceneWidget );
    splitter->addWidget( inspectorContainer );
    setCentralWidget( splitter );

    UpdateHierarchy();
    UpdateInspector();

    reloadTimer.setInterval( 3000 );
    connect( &reloadTimer, &QTimer::timeout, []() { ae3d::System::ReloadChangedAssets(); } );
    reloadTimer.start();
}

void MainWindow::OnGameObjectSelected( std::list< ae3d::GameObject* > /*gameObjects*/ )
{
    UpdateHierarchySelection();
    UpdateInspector();
}

void MainWindow::UpdateHierarchySelection()
{
    disconnect( sceneTree, &QTreeWidget::itemSelectionChanged, sceneTree, nullptr);
    disconnect( sceneTree, &QTreeWidget::itemChanged, sceneTree, nullptr);

    sceneTree->clearSelection();

    for (auto selectedIndex : sceneWidget->selectedGameObjects)
    {
        if (sceneTree->topLevelItem( selectedIndex ) != nullptr)
        {
            sceneTree->topLevelItem( selectedIndex )->setSelected( true );
        }
    }

    connect( sceneTree, &QTreeWidget::itemSelectionChanged, [&]() { HierarchySelectionChanged(); });
    connect( sceneTree, &QTreeWidget::itemChanged, [&](QTreeWidgetItem* item, int /* column */) { HierarchyItemRenamed( item ); });
}

void MainWindow::UpdateInspector()
{
    lightingInspector.GetWidget()->hide();

    if (sceneWidget->selectedGameObjects.empty())
    {
        transformInspector.GetWidget()->hide();
        cameraInspector.GetWidget()->hide();
        meshRendererInspector.GetWidget()->hide();
        dirLightInspector.GetWidget()->hide();
        spotLightInspector.GetWidget()->hide();
    }
    else
    {
        transformInspector.GetWidget()->show();

        auto cameraComponent = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() )->GetComponent< ae3d::CameraComponent >();
        if (cameraComponent)
        {
            cameraInspector.GetWidget()->show();
        }
        else
        {
            cameraInspector.GetWidget()->hide();
        }

        auto meshRendererComponent = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() )->GetComponent< ae3d::MeshRendererComponent >();
        if (meshRendererComponent)
        {
            meshRendererInspector.GetWidget()->show();
        }
        else
        {
            meshRendererInspector.GetWidget()->hide();
        }

        auto dirLightComponent = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() )->GetComponent< ae3d::DirectionalLightComponent >();
        if (dirLightComponent)
        {
            dirLightInspector.GetWidget()->show();
        }
        else
        {
            dirLightInspector.GetWidget()->hide();
        }

        auto spotLightComponent = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() )->GetComponent< ae3d::SpotLightComponent >();
        if (spotLightComponent)
        {
            spotLightInspector.GetWidget()->show();
        }
        else
        {
            spotLightInspector.GetWidget()->hide();
        }
    }
}

void MainWindow::OpenLightingInspector()
{
    transformInspector.GetWidget()->hide();
    cameraInspector.GetWidget()->hide();
    meshRendererInspector.GetWidget()->hide();
    dirLightInspector.GetWidget()->hide();
    spotLightInspector.GetWidget()->hide();
    lightingInspector.GetWidget()->show();
}

void MainWindow::ShowAbout()
{
    QMessageBox::about( this, "Controls", "Aether3D Editor by Timo Wiren 2016\n\nControls:\nRight mouse and W,S,A,D,Q,E: camera movement\nMiddle mouse: pan");
}

void MainWindow::HandleGameObjectsAddedOrDeleted()
{
    UpdateHierarchy();
}

void MainWindow::HierarchyItemRenamed( QTreeWidgetItem* item )
{
    int renamedIndex = 0;

    // Figures out which game object was renamed.
    for (int i = 0; i < sceneWidget->GetGameObjectCount(); ++i)
    {
        if (sceneTree->topLevelItem( i ) == item)
        {
            renamedIndex = i;
        }
    }

    sceneWidget->GetGameObject( renamedIndex )->SetName( item->text( 0 ).toUtf8().constData() );
    UpdateHierarchy();
}

void MainWindow::HierarchySelectionChanged()
{
    std::list< ae3d::GameObject* > selectedObjects;
    sceneWidget->selectedGameObjects.clear();

    if (sceneTree->selectedItems().empty())
    {
        UpdateInspector();
        emit GameObjectSelected( selectedObjects );
        return;
    }

    for (int g = 0; g < sceneWidget->GetGameObjectCount(); ++g)
    {
        for (auto i : sceneTree->selectedItems())
        {
            if (sceneTree->topLevelItem( g ) == i)
            {
                selectedObjects.push_back( sceneWidget->GetGameObject( g ) );
                sceneWidget->selectedGameObjects.push_back( g );
            }
        }
    }

    /*for (auto index : sceneWidget->selectedGameObjects)
    {
        std::cout << "selection: " << index << std::endl;
    }*/

    UpdateInspector();
    emit GameObjectSelected( selectedObjects );
}

void MainWindow::keyPressEvent( QKeyEvent* event )
{
    const int macDelete = 16777219;

    if (event->key() == Qt::Key_Escape)
    {
        std::list< ae3d::GameObject* > emptyList;
        emit GameObjectSelected( emptyList );
        sceneWidget->selectedGameObjects.clear();
        sceneTree->clearSelection();
        UpdateInspector();
    }
    else if (event->key() == Qt::Key_Delete || event->key() == macDelete)
    {
        std::vector< ae3d::GameObject* > selectedGameObjects;

        for (auto i : sceneWidget->selectedGameObjects)
        {
            selectedGameObjects.push_back( sceneWidget->GetGameObject( i ) );
        }

        for (auto g : selectedGameObjects)
        {
            sceneWidget->RemoveGameObject( g );
        }

        /*for (int i = 0; i < sceneTree->topLevelItemCount(); ++i)
        {
            if (sceneTree->topLevelItem( i )->isSelected())
            {
                sceneWidget->RemoveGameObject( i );
            }
        }*/

        sceneWidget->selectedGameObjects.clear();
        std::list< ae3d::GameObject* > emptyList;
        emit GameObjectSelected( emptyList );
        sceneTree->clearSelection();
        UpdateInspector();
        UpdateHierarchy();
    }
    else if (event->key() == Qt::Key_F)
    {
        sceneWidget->CenterSelected();
    }
    else if (event->key() == Qt::Key_D && (event->modifiers() & Qt::KeyboardModifier::ControlModifier) && !sceneWidget->selectedGameObjects.empty())
    {
        auto selected = sceneWidget->selectedGameObjects.back();
        std::cout << "Duplicate" << std::endl;
        commandManager.Execute( std::make_shared< CreateGoCommand >( sceneWidget ) );
        *sceneWidget->GetGameObject( sceneWidget->GetGameObjectCount() - 1 ) = *sceneWidget->GetGameObject( selected );
        UpdateHierarchy();
    }
}

void MainWindow::CommandCreateCameraComponent()
{
    commandManager.Execute( std::make_shared< CreateCameraCommand >( sceneWidget ) );
    sceneWidget->SetSelectedCameraTargetToPreview();
    UpdateInspector();
}

void MainWindow::CommandCreateMeshRendererComponent()
{
    commandManager.Execute( std::make_shared< CreateMeshRendererCommand >( sceneWidget ) );
    UpdateInspector();
}

void MainWindow::CommandCreateDirectionalLightComponent()
{
    commandManager.Execute( std::make_shared< CreateLightCommand >( sceneWidget, CreateLightCommand::Type::Directional ) );
    UpdateInspector();
}

void MainWindow::CommandCreateSpotLightComponent()
{
    commandManager.Execute( std::make_shared< CreateLightCommand >( sceneWidget, CreateLightCommand::Type::Spot ) );
    UpdateInspector();
}

void MainWindow::CommandModifyTransform( int gameObjectIndex, const ae3d::Vec3& newPosition, const ae3d::Quaternion& newRotation, float newScale )
{
    commandManager.Execute( std::make_shared< ModifyTransformCommand >( gameObjectIndex, sceneWidget, newPosition, newRotation, newScale ) );
    sceneWidget->UpdateTransformGizmoPosition();
}

void MainWindow::CommandCreateGameObject()
{
    commandManager.Execute( std::make_shared< CreateGoCommand >( sceneWidget ) );
    UpdateHierarchy();
}

void MainWindow::CommandModifyCamera( ae3d::CameraComponent::ClearFlag clearFlag, ae3d::CameraComponent::ProjectionType projectionType,
                                      const ae3d::Vec4& orthoParams, const ae3d::Vec4& perspParams, const ae3d::Vec3& clearColor )
{
    ae3d::System::Assert( !sceneWidget->selectedGameObjects.empty(), "Cannot modify a camera if selection is empty" );

    auto camera = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() )->GetComponent< ae3d::CameraComponent >();
    ae3d::System::Assert( camera, "First selected object doesn't contain a camera component" );
    commandManager.Execute( std::make_shared< ModifyCameraCommand >( camera, clearFlag, projectionType, orthoParams, perspParams, clearColor ) );
}

void MainWindow::LoadScene()
{
    const QString fileName = QFileDialog::getOpenFileName( this, "Open Scene", "", "Scenes (*.scene)" );

    if (!fileName.isEmpty())
    {
        // TODO: Clear scene.
        // TODO: Ask for confirmation if the scene has unsaved modifications.
        sceneWidget->LoadSceneFromFile( fileName.toUtf8().constData() );
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
    sceneWidget->RemoveEditorObjects();
    ofs << sceneWidget->GetScene()->GetSerialized();
    sceneWidget->AddEditorObjects();
    const bool saveSuccess = ofs.is_open();

    if (!saveSuccess && !fileName.isEmpty())
    {
        QMessageBox::critical( this, "Unable to Save!", "Scene could not be saved." );
    }
}

void MainWindow::UpdateHierarchy()
{
    sceneTree->clear();
    sceneTree->setHeaderLabel( "Hierarchy" );

    QList< QTreeWidgetItem* > nodes;
    const int count = sceneWidget->GetGameObjectCount();

    for (int i = 0; i < count; ++i)
    {
        nodes.append(new QTreeWidgetItem( (QTreeWidget*)0, QStringList( QString( sceneWidget->GetGameObject( i )->GetName().c_str() ) ) ) );
        nodes.back()->setFlags( nodes.back()->flags() | Qt::ItemIsEditable );
        //std::cout << "added to list: " <<  sceneWidget->GetGameObject( i )->GetName()<< std::endl;
    }

    sceneTree->insertTopLevelItems( 0, nodes );
}
