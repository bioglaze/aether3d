#ifndef MAINWINDOW
#define MAINWINDOW

#include <list>
#include <QMainWindow>
#include "WindowMenu.hpp"
#include "CommandManager.hpp"

class SceneWidget;
class QTreeWidget;
class QKeyEvent;
class QTreeWidgetItem;

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

signals:
    void GameObjectSelected( std::list< ae3d::GameObject* > gameObjects );

private:
    void UpdateHierarchy();
    void keyPressEvent( QKeyEvent* event );
    void SelectTreeItem( QTreeWidgetItem* item );
    void HierarchyItemRenamed( QTreeWidgetItem* item );

    QTreeWidget* sceneTree = nullptr;
    SceneWidget* sceneWidget = nullptr;
    WindowMenu windowMenu;
    CommandManager commandManager;
};

#endif

