#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include <QBoxLayout>
#include <QMenu>
#include <QSplitter>
#include <QTreeWidget>
#include <QKeyEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QWindow>
#include <QScrollArea>
#include "AudioSourceComponent.hpp"
#include "CameraComponent.hpp"
#include "CreateAudioSourceCommand.hpp"
#include "CreateCameraCommand.hpp"
#include "CreateGoCommand.hpp"
#include "CreateLightCommand.hpp"
#include "CreateMeshRendererCommand.hpp"
#include "CreateSpriteRendererCommand.hpp"
#include "DirectionalLightComponent.hpp"
#include "MainWindow.hpp"
#include "MeshRendererComponent.hpp"
#include "ModifyTransformCommand.hpp"
#include "ModifyCameraCommand.hpp"
#include "ModifySpriteRendererCommand.hpp"
#include "PointLightComponent.hpp"
#include "Quaternion.hpp"
#include "RemoveComponentCommand.hpp"
#include "RenameGameObjectCommand.hpp"
#include "SceneWidget.hpp"
#include "System.hpp"
#include "SpotLightComponent.hpp"
#include "TransformInspector.hpp"
#include "WindowMenu.hpp"

//#define AE3D_RUN_UNIT_TESTS 1

#if AE3D_RUN_UNIT_TESTS
void TestCommands( MainWindow* mainWindow, SceneWidget* sceneWidget )
{
    using namespace ae3d;

    mainWindow->CommandCreateGameObject();
    System::Assert( sceneWidget->selectedGameObjects.size() == 1, "adding game object didn't update selection correctly" );

    mainWindow->CommandCreateAudioSourceComponent();
    System::Assert( sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() )->GetComponent< AudioSourceComponent >(), "audio source component was not added" );

    System::Print( "ran unit tests\n" );
    exit( 0 );
}
#endif

MainWindow::MainWindow()
{
    sceneTree = new QTreeWidget();
    sceneTree->setColumnCount( 1 );
    connect( sceneTree, &QTreeWidget::itemSelectionChanged, [&]() { HierarchySelectionChanged(); });
    connect( sceneTree, &QTreeWidget::itemChanged, [&](QTreeWidgetItem* item, int /* column */) { HierarchyItemRenamed( item ); });
    sceneTree->setSelectionMode( QAbstractItemView::SelectionMode::ExtendedSelection );
    sceneTree->setContextMenuPolicy( Qt::CustomContextMenu );
    connect( sceneTree, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowContextMenu(const QPoint&)));

    sceneWidget = new SceneWidget();
    sceneWidget->SetMainWindow( this );
    setWindowTitle( "Editor" );

    connect( sceneWidget, &SceneWidget::GameObjectsAddedOrDeleted, this, &MainWindow::HandleGameObjectsAddedOrDeleted );
    connect( &transformInspector, &TransformInspector::TransformModified, this, &MainWindow::CommandModifyTransform );
    connect( sceneWidget, &SceneWidget::TransformModified, this, &MainWindow::CommandModifyTransform );
    connect( this, &MainWindow::GameObjectSelected, this, &MainWindow::OnGameObjectSelected );
    connect( &cameraInspector, &CameraInspector::CameraModified, this, &MainWindow::CommandModifyCamera );
    connect( &spriteRendererInspector, &SpriteRendererInspector::SpriteRendererModified, this, &MainWindow::CommandModifySpriteRenderer );

    windowMenu.Init( this );
    setMenuBar( windowMenu.menuBar );

    transformInspector.Init( this );
    cameraInspector.Init( this );
    meshRendererInspector.Init( this, sceneWidget );
    dirLightInspector.Init( this );
    pointLightInspector.Init( this );
    spotLightInspector.Init( this );
    spriteRendererInspector.Init( this );
    audioSourceInspector.Init( this );
    lightingInspector.Init( sceneWidget );

    QBoxLayout* inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->addWidget( transformInspector.GetWidget() );
    inspectorLayout->addWidget( cameraInspector.GetWidget() );
    inspectorLayout->addWidget( meshRendererInspector.GetWidget() );
    inspectorLayout->addWidget( dirLightInspector.GetWidget() );
    inspectorLayout->addWidget( pointLightInspector.GetWidget() );
    inspectorLayout->addWidget( spotLightInspector.GetWidget() );
    inspectorLayout->addWidget( audioSourceInspector.GetWidget() );
    inspectorLayout->addWidget( spriteRendererInspector.GetWidget() );
    inspectorLayout->addWidget( lightingInspector.GetWidget() );

    inspectorContainer = new QWidget();
    inspectorContainer->setLayout( inspectorLayout );
    inspectorContainer->setMinimumWidth( 380 );
    inspectorContainer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

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

#if AE3D_RUN_UNIT_TESTS
    TestCommands( this, sceneWidget );
#endif
}

void MainWindow::closeEvent( QCloseEvent *event )
{
    if (commandManager.HasUnsavedChanges())
    {
        switch( QMessageBox::information( this, "Qt Application Example",
                                              "The document has been changed since "
                                              "the last save.",
                                              "Save Now", "Cancel", "Leave Anyway",
                                              0, 1 ) )
        {
            case 0:
                SaveScene();
                event->accept();
                break;
            case 1:
            default: // just for sanity
                event->ignore();
                break;
            case 2:
                event->accept();
                exit( 0 );
                break;
        }
    }
}

void MainWindow::ShowContextMenu( const QPoint& pos )
{
    if (sceneTree->selectedItems().empty())
    {
        return;
    }

    QPoint globalPos = sceneTree->mapToGlobal( pos );

    QMenu myMenu;
    myMenu.addAction( "Rename" );
    myMenu.addAction( "Duplicate" );
    myMenu.addAction( "Delete" );

    QAction* selectedItem = myMenu.exec( globalPos );

    if (selectedItem && sceneTree->selectedItems().length() == 1)
    {
        if (selectedItem->text() == "Rename")
        {
            sceneTree->editItem( sceneTree->selectedItems().front() );
        }
        else if (selectedItem->text() == "Duplicate")
        {
            auto selected = sceneWidget->selectedGameObjects.back();
            std::cout << "Duplicate" << std::endl;
            commandManager.Execute( std::make_shared< CreateGoCommand >( sceneWidget ) );
            *sceneWidget->GetGameObject( sceneWidget->GetGameObjectCount() - 1 ) = *sceneWidget->GetGameObject( selected );
            UpdateHierarchy();
        }
        else if (selectedItem->text() == "Delete")
        {
            DeleteSelectedGameObjects();
        }
    }
    else
    {
        // nothing was chosen
    }
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
        pointLightInspector.GetWidget()->hide();
        spotLightInspector.GetWidget()->hide();
        audioSourceInspector.GetWidget()->hide();
        spriteRendererInspector.GetWidget()->hide();
    }
    else
    {
        transformInspector.GetWidget()->show();

        auto cameraComponent = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() )->GetComponent< ae3d::CameraComponent >();
        if (cameraComponent)
        {
            cameraInspector.GetWidget()->show();
            cameraInspector.ApplySelectedCameraIntoFields( *cameraComponent );
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

        auto pointLightComponent = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() )->GetComponent< ae3d::PointLightComponent >();
        if (pointLightComponent)
        {
            pointLightInspector.GetWidget()->show();
        }
        else
        {
            pointLightInspector.GetWidget()->hide();
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

        auto audioSourceComponent = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() )->GetComponent< ae3d::AudioSourceComponent >();
        if (audioSourceComponent)
        {
            audioSourceInspector.GetWidget()->show();
        }
        else
        {
            audioSourceInspector.GetWidget()->hide();
        }

        auto spriteRendererComponent = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() )->GetComponent< ae3d::SpriteRendererComponent >();
        if (spriteRendererComponent)
        {
            spriteRendererInspector.GetWidget()->show();
        }
        else
        {
            spriteRendererInspector.GetWidget()->hide();
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
    pointLightInspector.GetWidget()->hide();
    spriteRendererInspector.GetWidget()->hide();
    audioSourceInspector.GetWidget()->hide();
    lightingInspector.GetWidget()->show();
}

void MainWindow::ShowAbout()
{
    QMessageBox::about( this, "About", "Aether3D Editor by Timo Wiren 2016\n\nControls:\n\
Right mouse and W,S,A,D,Q,E: camera movement\nMiddle mouse: pan\nCtrl-D: duplicate\nF: Focus on selected");
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

    if (item->text( 0 ).length() > 0)
    {
        std::string name = item->text( 0 ).toStdString();
        bool nameContainsOnlyWhiteSpace = true;

        for (std::size_t charInd = 0; charInd < name.length(); ++charInd)
        {
            if (name.at( charInd ) != ' ' && name.at( charInd ) != '\t')
            {
                nameContainsOnlyWhiteSpace = false;
                break;
            }
        }

        if (nameContainsOnlyWhiteSpace)
        {
            QString oldName( sceneWidget->GetGameObject( renamedIndex )->GetName().c_str() );
            item->setText( 0, oldName );
        }
        else
        {
            CommandRenameGameObject( sceneWidget->GetGameObject( renamedIndex ), name );
        }
    }

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

void MainWindow::DeleteSelectedGameObjects()
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
        DeleteSelectedGameObjects();
    }
    else if (event->key() == Qt::Key_F)
    {
        sceneWidget->CenterSelected();
    }
    else if (event->key() == Qt::Key_D && (event->modifiers() & Qt::KeyboardModifier::ControlModifier) && !sceneWidget->selectedGameObjects.empty())
    {
        DuplicateSelected();
    }
}

void MainWindow::DuplicateSelected()
{
    auto selected = sceneWidget->selectedGameObjects.back();
    std::cout << "Duplicate" << std::endl;
    commandManager.Execute( std::make_shared< CreateGoCommand >( sceneWidget ) );
    *sceneWidget->GetGameObject( sceneWidget->GetGameObjectCount() - 1 ) = *sceneWidget->GetGameObject( selected );
    UpdateHierarchy();
}

void MainWindow::CommandRenameGameObject( ae3d::GameObject* gameObject, const std::string& newName )
{
    if (!sceneWidget->selectedGameObjects.empty())
    {
        commandManager.Execute( std::make_shared< RenameGameObjectCommand >( gameObject, newName ) );
    }
}

void MainWindow::CommandCreateAudioSourceComponent()
{
    if (!sceneWidget->selectedGameObjects.empty())
    {
        commandManager.Execute( std::make_shared< CreateAudioSourceCommand >( sceneWidget ) );
        UpdateInspector();
    }
}

void MainWindow::CommandRemoveAudioSourceComponent()
{
    auto component = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.back() )->GetComponent< ae3d::AudioSourceComponent >();
    commandManager.Execute( std::make_shared< RemoveComponentCommand >( component ) );
    UpdateInspector();
}

void MainWindow::CommandCreateCameraComponent()
{
    if (!sceneWidget->selectedGameObjects.empty())
    {
        commandManager.Execute( std::make_shared< CreateCameraCommand >( sceneWidget ) );
        sceneWidget->SetSelectedCameraTargetToPreview();
        UpdateInspector();
    }
}

void MainWindow::CommandRemoveCameraComponent()
{
    auto component = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.back() )->GetComponent< ae3d::CameraComponent >();
    commandManager.Execute( std::make_shared< RemoveComponentCommand >( component ) );
    UpdateInspector();
    sceneWidget->HideHUD();
}

void MainWindow::CommandCreateMeshRendererComponent()
{
    if (!sceneWidget->selectedGameObjects.empty())
    {
        commandManager.Execute( std::make_shared< CreateMeshRendererCommand >( sceneWidget ) );
        UpdateInspector();
    }
}

void MainWindow::CommandRemoveMeshRendererComponent()
{
    auto component = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.back() )->GetComponent< ae3d::MeshRendererComponent >();
    commandManager.Execute( std::make_shared< RemoveComponentCommand >( component ) );
    UpdateInspector();
}

void MainWindow::CommandCreateSpriteRendererComponent()
{
    if (!sceneWidget->selectedGameObjects.empty())
    {
        commandManager.Execute( std::make_shared< CreateSpriteRendererCommand >( sceneWidget ) );
        UpdateInspector();
    }
}

void MainWindow::CommandRemoveSpriteRendererComponent()
{
    auto component = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.back() )->GetComponent< ae3d::SpriteRendererComponent >();
    commandManager.Execute( std::make_shared< RemoveComponentCommand >( component ) );
    UpdateInspector();
}

void MainWindow::CommandCreateDirectionalLightComponent()
{
    if (!sceneWidget->selectedGameObjects.empty())
    {
        commandManager.Execute( std::make_shared< CreateLightCommand >( sceneWidget, CreateLightCommand::Type::Directional ) );
        UpdateInspector();
    }
}

void MainWindow::CommandRemoveDirectionalLightComponent()
{
    auto component = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.back() )->GetComponent< ae3d::DirectionalLightComponent >();
    commandManager.Execute( std::make_shared< RemoveComponentCommand >( component ) );
    UpdateInspector();
}

void MainWindow::CommandCreateSpotLightComponent()
{
    if (!sceneWidget->selectedGameObjects.empty())
    {
        commandManager.Execute( std::make_shared< CreateLightCommand >( sceneWidget, CreateLightCommand::Type::Spot ) );
        UpdateInspector();
    }
}

void MainWindow::CommandRemoveSpotLightComponent()
{
    auto component = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.back() )->GetComponent< ae3d::SpotLightComponent >();
    commandManager.Execute( std::make_shared< RemoveComponentCommand >( component ) );
    UpdateInspector();
}

void MainWindow::CommandCreatePointLightComponent()
{
    if (!sceneWidget->selectedGameObjects.empty())
    {
        commandManager.Execute( std::make_shared< CreateLightCommand >( sceneWidget, CreateLightCommand::Type::Point ) );
        UpdateInspector();
    }
}

void MainWindow::CommandRemovePointLightComponent()
{
    auto component = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.back() )->GetComponent< ae3d::PointLightComponent >();
    commandManager.Execute( std::make_shared< RemoveComponentCommand >( component ) );
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
                                      const ae3d::Vec4& orthoParams, const ae3d::Vec4& perspParams, const ae3d::Vec3& clearColor,
                                      int renderOrder )
{
    ae3d::System::Assert( !sceneWidget->selectedGameObjects.empty(), "Cannot modify a camera if selection is empty" );

    auto camera = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() )->GetComponent< ae3d::CameraComponent >();
    ae3d::System::Assert( camera, "First selected object doesn't contain a camera component" );
    commandManager.Execute( std::make_shared< ModifyCameraCommand >( camera, clearFlag, projectionType, orthoParams, perspParams,
                                                                     clearColor, renderOrder ) );
}

void MainWindow::CommandModifySpriteRenderer( std::string path, float x, float y, float width, float height )
{
    ae3d::System::Assert( !sceneWidget->selectedGameObjects.empty(), "Cannot modify a sprite renderer if selection is empty" );

    auto spriteRenderer = sceneWidget->GetGameObject( sceneWidget->selectedGameObjects.front() )->GetComponent< ae3d::SpriteRendererComponent >();
    ae3d::System::Assert( spriteRenderer, "First selected object doesn't contain a sprite renderer component" );
    commandManager.Execute( std::make_shared< ModifySpriteRendererCommand >( spriteRenderer, path, x, y, width, height ) );
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
    else
    {
        commandManager.SceneSaved();
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
