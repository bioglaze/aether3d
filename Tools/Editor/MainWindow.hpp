#ifndef MAINWINDOW
#define MAINWINDOW

#include <list>
#include <QMainWindow>
#include "WindowMenu.hpp"
#include "CommandManager.hpp"
#include "CameraInspector.hpp"
#include "TransformInspector.hpp"

class SceneWidget;
class QTreeWidget;
class QKeyEvent;
class QTreeWidgetItem;
class QWidget;

namespace ae3d
{
    class GameObject;
    class TransformComponent;
    struct Vec3;
    struct Quaternion;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    SceneWidget* GetSceneWidget() const { return sceneWidget; }

public slots:
    void LoadScene();
    void SaveScene();
    void CommandCreateCameraComponent();
    void CommandCreateGameObject();
    void CommandCreateMeshRendererComponent();
    void CommandModifyTransform( const ae3d::Vec3& newPosition, const ae3d::Quaternion& newRotation, float newScale );
    void CommandModifyCamera( ae3d::CameraComponent::ClearFlag clearFlag, const ae3d::Vec4& orthoParams, const ae3d::Vec4& perspParams );
    void Undo() { commandManager.Undo(); UpdateHierarchy(); }
    void HandleGameObjectsAddedOrDeleted();
    void ShowAbout();

private slots:
    void OnGameObjectSelected( std::list< ae3d::GameObject* > gameObjects );

signals:
    void GameObjectSelected( std::list< ae3d::GameObject* > gameObjects );

private:
    void UpdateHierarchy();
    void UpdateHierarchySelection();
    void keyPressEvent( QKeyEvent* event );
    void HierarchySelectionChanged();
    void HierarchyItemRenamed( QTreeWidgetItem* item );
    void UpdateInspector();

    QWidget* inspectorContainer = nullptr;
    QTreeWidget* sceneTree = nullptr;
    SceneWidget* sceneWidget = nullptr;
    TransformInspector transformInspector;
    CameraInspector cameraInspector;
    WindowMenu windowMenu;
    CommandManager commandManager;
};

#endif

