#ifndef MAINWINDOW
#define MAINWINDOW

#include <list>
#include <QMainWindow>
#include "WindowMenu.hpp"
#include "CommandManager.hpp"
#include "TransformInspector.hpp"

class SceneWidget;
class QTreeWidget;
class QKeyEvent;
class QTreeWidgetItem;
class QWidget;

namespace ae3d
{
    class GameObject;
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
    void Undo() { commandManager.Undo(); UpdateHierarchy(); }
    void HandleGameObjectsAddedOrDeleted();
    void ShowAbout();

signals:
    void GameObjectSelected( std::list< ae3d::GameObject* > gameObjects );

private:
    void UpdateHierarchy();
    void keyPressEvent( QKeyEvent* event );
    void SelectTreeItem( QTreeWidgetItem* item );
    void HierarchyItemRenamed( QTreeWidgetItem* item );
    void UpdateInspector();

    QWidget* inspectorContainer = nullptr;
    QTreeWidget* sceneTree = nullptr;
    SceneWidget* sceneWidget = nullptr;
    TransformInspector transformInspector;
    WindowMenu windowMenu;
    CommandManager commandManager;
};

#endif

