#ifndef TRANSFORMINSPECTOR_HPP
#define TRANSFORMINSPECTOR_HPP

#include <QObject>

namespace ae3d
{
    class GameObject;
    struct Vec3;
    struct Quaternion;
}

class QTableWidget;
class QTableWidgetItem;
class QWidget;

class TransformInspector: public QObject
{
    Q_OBJECT

  public:
    QWidget* GetWidget() { return reinterpret_cast<QWidget*>(table); }
    void Init( QWidget* mainWindow );

signals:
    void TransformModified( int gameObjectIndex, const ae3d::Vec3& newPosition, const ae3d::Quaternion& newRotation, float newScale );

private slots:
    void GameObjectSelected( std::list< ae3d::GameObject* > gameObjects );

private:
    void FieldsChanged( QTableWidgetItem* item );

    QTableWidget* table = nullptr;
    class SceneWidget* sceneWidget = nullptr; // Owner: MainWindow
};
#endif
