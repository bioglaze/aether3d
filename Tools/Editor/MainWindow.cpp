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
#include "CameraComponent.hpp"
#include "DirectionalLightComponent.hpp"
#include "MainWindow.hpp"
#include "SceneWidget.hpp"
#include "System.hpp"
#include "WindowMenu.hpp"
#include "CreateCameraCommand.hpp"
#include "CreateGoCommand.hpp"
#include "CreateLightCommand.hpp"
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
    connect( sceneWidget, SIGNAL(GameObjectsAddedOrDeleted()), this, SLOT(HandleGameObjectsAddedOrDeleted()) );

    connect( &transformInspector, SIGNAL(TransformModified( const ae3d::Vec3&, const ae3d::Quaternion&, float )),
             this, SLOT(CommandModifyTransform( const ae3d::Vec3&, const ae3d::Quaternion&, float )) );

    connect( sceneWidget, SIGNAL(TransformModified( const ae3d::Vec3&, const ae3d::Quaternion&, float )),
             this, SLOT(CommandModifyTransform( const ae3d::Vec3&, const ae3d::Quaternion&, float )) );

    connect(this, SIGNAL(GameObjectSelected(std::list< ae3d::GameObject* >)),
            this, SLOT(OnGameObjectSelected(std::list< ae3d::GameObject* >)));

    connect( &cameraInspector, SIGNAL(CameraModified(ae3d::CameraComponent::ClearFlag, ae3d::CameraComponent::ProjectionType, const ae3d::Vec4&, const ae3d::Vec4&, const ae3d::Vec3&)),
             this, SLOT(CommandModifyCamera(ae3d::CameraComponent::ClearFlag, ae3d::CameraComponent::ProjectionType, const ae3d::Vec4&, const ae3d::Vec4&, const ae3d::Vec3&)) );

    windowMenu.Init( this );
    setMenuBar( windowMenu.menuBar );

    transformInspector.Init( this );
    cameraInspector.Init( this );
    meshRendererInspector.Init( this );
    dirLightInspector.Init( this );

    QBoxLayout* inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->addWidget( transformInspector.GetWidget() );
    inspectorLayout->addWidget( cameraInspector.GetWidget() );
    inspectorLayout->addWidget( meshRendererInspector.GetWidget() );
    inspectorLayout->addWidget( dirLightInspector.GetWidget() );

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

    reloadTimer.setInterval( 1000 );
    connect( &reloadTimer, &QTimer::timeout, []() { ae3d::System::ReloadChangedAssets(); std::cout << "Reloading assets" << std::endl; } );
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
    if (sceneWidget->selectedGameObjects.empty())
    {
        transformInspector.GetWidget()->hide();
        cameraInspector.GetWidget()->hide();
        meshRendererInspector.GetWidget()->hide();
        dirLightInspector.GetWidget()->hide();
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
    }
}

void MainWindow::ShowAbout()
{
    QMessageBox::about( this, "Controls", "Aether3D Editor by Timo Wiren 2015\n\nControls:\nRight mouse and W,S,A,D,Q,E: camera movement\nMiddle mouse: pan");
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
        for (int i = 0; i < sceneTree->topLevelItemCount(); ++i)
        {
            if (sceneTree->topLevelItem( i )->isSelected())
            {
                sceneWidget->RemoveGameObject( i );
                sceneWidget->selectedGameObjects.clear();
                std::list< ae3d::GameObject* > emptyList;
                emit GameObjectSelected( emptyList );
                sceneTree->clearSelection();
                UpdateInspector();
            }
        }

        /*for (auto goIndex : sceneWidget->selectedGameObjects)
        {
            std::cout << "removed " << sceneWidget->GetGameObject(goIndex)->GetName() << std::endl;
            sceneWidget->RemoveGameObject( goIndex );
            sceneWidget->selectedGameObjects.clear();
        }*/

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
    UpdateInspector();
}

void MainWindow::CommandCreateMeshRendererComponent()
{
    //commandManager.Execute( std::make_shared< CreateMeshRendererCommand >( sceneWidget ) );
    UpdateInspector();
}

void MainWindow::CommandCreateDirectionalLightComponent()
{
    commandManager.Execute( std::make_shared< CreateLightCommand >( sceneWidget ) );
    UpdateInspector();
}

void MainWindow::CommandModifyTransform( const ae3d::Vec3& newPosition, const ae3d::Quaternion& newRotation, float newScale )
{
    commandManager.Execute( std::make_shared< ModifyTransformCommand >( sceneWidget, newPosition, newRotation, newScale ) );
}

void MainWindow::CommandCreateGameObject()
{
    commandManager.Execute( std::make_shared< CreateGoCommand >( sceneWidget ) );
    UpdateHierarchy();
}

void MainWindow::CommandModifyCamera( ae3d::CameraComponent::ClearFlag clearFlag, ae3d::CameraComponent::ProjectionType projectionType,
                                      const ae3d::Vec4& orthoParams, const ae3d::Vec4& perspParams, const ae3d::Vec3& clearColor )
{
    auto camera = sceneWidget->GetGameObject( 0 )->GetComponent< ae3d::CameraComponent >();
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
