#ifndef CAMERAINSPECTOR_HPP
#define CAMERAINSPECTOR_HPP

#include <list>
#include <QObject>
#include "CameraComponent.hpp"

namespace ae3d
{
    class GameObject;
    struct Vec3;
    struct Vec4;
    struct Quaternion;
}

class QTableWidget;
class QTableWidgetItem;
class QWidget;
class QComboBox;
class QString;

class CameraInspector : public QObject
{
    Q_OBJECT

  public:
    QWidget* GetWidget() { return root; }
    void Init( QWidget* mainWindow );

signals:
    void CameraModified( ae3d::CameraComponent::ClearFlag clearFlag, const ae3d::Vec4& orthoParams, const ae3d::Vec4& perspParams );

private slots:
    void GameObjectSelected( std::list< ae3d::GameObject* > gameObjects );
    void ClearFlagsChanged( int item );
    void ProjectionChanged();

private:
    void ApplyFieldsIntoSelectedCamera();
    void ApplySelectedCameraIntoFields( const ae3d::CameraComponent& camera );

    QWidget* root = nullptr;
    QTableWidget* ortho = nullptr;
    QTableWidget* persp = nullptr;
    QComboBox* clearFlagsBox = nullptr;
    QComboBox* projectionBox = nullptr;
};
#endif
