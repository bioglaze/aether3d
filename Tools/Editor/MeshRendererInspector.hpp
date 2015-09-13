#ifndef MESHRENDERERINSPECTOR_HPP
#define MESHRENDERERINSPECTOR_HPP

#include <QObject>
#include <list>

namespace ae3d
{
    class GameObject;
}

class QTableWidget;
class QWidget;

class MeshRendererInspector : public QObject
{
    Q_OBJECT
public:
    QWidget* GetWidget() { return root; }
    void Init( QWidget* mainWindow );

private slots:
    void MeshCellClicked( int, int );
    void GameObjectSelected( std::list< ae3d::GameObject* > gameObjects );

private:
    QWidget* root = nullptr;
    QTableWidget* meshTable = nullptr;
    QWidget* mainWindow = nullptr;
    ae3d::GameObject* gameObject = nullptr;
};

#endif
