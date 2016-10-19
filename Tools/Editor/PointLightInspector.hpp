#ifndef POINTLIGHTINSPECTOR_HPP
#define POINTLIGHTINSPECTOR_HPP

#include <list>
#include <QObject>

namespace ae3d
{
    class GameObject;
}

class QWidget;

class PointLightInspector : public QObject
{
    Q_OBJECT

  public:
    QWidget* GetWidget() { return root; }
    void Init( QWidget* mainWindow );

private slots:
    void ShadowStateChanged( int );
    void GameObjectSelected( std::list< ae3d::GameObject* > gameObjects );

  private:
    QWidget* root = nullptr;
    class QCheckBox* shadowCheck = nullptr;
    class QPushButton* removeButton = nullptr;
    ae3d::GameObject* gameObject = nullptr;
};

#endif
