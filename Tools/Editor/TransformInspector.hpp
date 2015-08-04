#ifndef TRANSFORMINSPECTOR_HPP
#define TRANSFORMINSPECTOR_HPP

//#include <list>
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
    QWidget* GetWidget() { return (QWidget*)table; }
    void Init( QWidget* mainWindow );

signals:
    void TransformModified( const ae3d::Vec3& newPosition, const ae3d::Quaternion& newRotation, float newScale );

private slots:
    void GameObjectSelected( std::list< ae3d::GameObject* > gameObjects );

private:
    void FieldsChanged( QTableWidgetItem* item );

    QTableWidget* table = nullptr;
};
#endif
