#ifndef MAINWINDOW
#define MAINWINDOW

#include <list>
#include <QMainWindow>
#include <QTimer>
#include "AudioSourceInspector.hpp"
#include "CommandManager.hpp"
#include "CameraInspector.hpp"
#include "DirectionalLightInspector.hpp"
#include "LightingInspector.hpp"
#include "MeshRendererInspector.hpp"
#include "PointLightInspector.hpp"
#include "SpotLightInspector.hpp"
#include "SpriteRendererInspector.hpp"
#include "TransformInspector.hpp"
#include "WindowMenu.hpp"

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
    class SceneWidget* GetSceneWidget() const { return sceneWidget; }

public slots:
    void LoadScene();
    void SaveScene();

    void CommandCreateAudioSourceComponent();
    void CommandCreateCameraComponent();
    void CommandCreateGameObject();
    void CommandCreateMeshRendererComponent();
    void CommandCreateSpriteRendererComponent();
    void CommandCreateDirectionalLightComponent();
    void CommandCreateSpotLightComponent();
    void CommandCreatePointLightComponent();

    void CommandRemoveAudioSourceComponent();
    void CommandRemoveCameraComponent();
    void CommandRemoveMeshRendererComponent();
    void CommandRemoveSpriteRendererComponent();
    void CommandRemoveDirectionalLightComponent();
    void CommandRemoveSpotLightComponent();
    void CommandRemovePointLightComponent();

    void CommandModifyTransform( int gameObjectIndex, const ae3d::Vec3& newPosition, const ae3d::Quaternion& newRotation, float newScale );
    void CommandModifyCamera( ae3d::CameraComponent::ClearFlag clearFlag, ae3d::CameraComponent::ProjectionType projectionType,
                              const ae3d::Vec4& orthoParams, const ae3d::Vec4& perspParams, const ae3d::Vec3& clearColor );

    void OpenLightingInspector();
    void Undo() { commandManager.Undo(); UpdateHierarchy(); }
    void HandleGameObjectsAddedOrDeleted();
    void ShowAbout();

private slots:
    void OnGameObjectSelected( std::list< ae3d::GameObject* > gameObjects );
    void ShowContextMenu(const class QPoint& pos);

signals:
    void GameObjectSelected( std::list< ae3d::GameObject* > gameObjects );

private:
    void DeleteSelectedGameObjects();
    void UpdateHierarchy();
    void UpdateHierarchySelection();
    void keyPressEvent( QKeyEvent* event );
    void HierarchySelectionChanged();
    void HierarchyItemRenamed( QTreeWidgetItem* item );
    void UpdateInspector();

    QWidget* inspectorContainer = nullptr;
    QTreeWidget* sceneTree = nullptr;
    QTimer reloadTimer;
    SceneWidget* sceneWidget = nullptr;
    TransformInspector transformInspector;
    CameraInspector cameraInspector;
    MeshRendererInspector meshRendererInspector;
    DirectionalLightInspector dirLightInspector;
    PointLightInspector pointLightInspector;
    SpotLightInspector spotLightInspector;
    LightingInspector lightingInspector;
    AudioSourceInspector audioSourceInspector;
    SpriteRendererInspector spriteRendererInspector;
    WindowMenu windowMenu;
    CommandManager commandManager;
};

#endif

