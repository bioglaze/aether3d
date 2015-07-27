#ifndef TRANSFORMINSPECTOR_HPP
#define TRANSFORMINSPECTOR_HPP

#include <list>
#include <QObject>

namespace ae3d
{
    class GameObject;
}

class QTableWidget;
class QTableWidgetItem;
class QWidget;

class TransformInspector: QObject
{
    Q_OBJECT

  public:
    QWidget* GetWidget() { return (QWidget*)table; }
    void Init( QWidget* mainWindow );

private slots:
    void GameObjectSelected( std::list< ae3d::GameObject* > gameObjects );

private:
    void FieldsChanged( QTableWidgetItem* item );

    QTableWidget* table = nullptr;
    std::list< ae3d::GameObject* > selectedGameObjects;
};
#endif
