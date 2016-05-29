#ifndef MESHRENDERERINSPECTOR_HPP
#define MESHRENDERERINSPECTOR_HPP

#include <QObject>
#include <list>

namespace ae3d
{
    class GameObject;
}

class MeshRendererInspector : public QObject
{
    Q_OBJECT
public:
    class QWidget* GetWidget() { return root; }
    void Init( QWidget* mainWindow, class SceneWidget* sceneWidget );

private slots:
    void MeshCellClicked( int, int );
    void GameObjectSelected( std::list< ae3d::GameObject* > gameObjects );

private:
    QWidget* root = nullptr;
    class QTableWidget* meshTable = nullptr;
    QWidget* mainWindow = nullptr;
    class QPushButton* removeButton = nullptr;
    ae3d::GameObject* gameObject = nullptr;
    SceneWidget* sceneWidget = nullptr;
};

#endif
