#ifndef SPRITERENDERERINSPECTOR_HPP
#define SPRITERENDERERINSPECTOR_HPP

#include <list>
#include <string>
#include <QObject>

namespace ae3d
{
    class SpriteRendererComponent;
    class GameObject;
}

class SpriteRendererInspector : public QObject
{
    Q_OBJECT

  public:
    class QWidget* GetWidget() { return root; }
    void Init( QWidget* mainWindow );

signals:
    void SpriteRendererModified( std::string path, float x, float y, float width, float height );

private slots:
    void GameObjectSelected( std::list< ae3d::GameObject* > gameObjects );

  private:
    void ApplyFieldsIntoSelectedSpriteRenderer();
    void ApplySelectedSpriteRendererIntoFields( const ae3d::SpriteRendererComponent& spriteRenderer );

    QWidget* root = nullptr;
    class QPushButton* removeButton = nullptr;
    class QTableWidget* table = nullptr;
};

#endif
