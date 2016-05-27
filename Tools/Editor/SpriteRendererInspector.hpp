#ifndef SPRITERENDERERINSPECTOR_HPP
#define SPRITERENDERERINSPECTOR_HPP

#include <QObject>

class SpriteRendererInspector : public QObject
{
    Q_OBJECT

  public:
    class QWidget* GetWidget() { return root; }
    void Init( QWidget* mainWindow );

  private:
    QWidget* root = nullptr;
    class QPushButton* removeButton = nullptr;
    class QTableWidget* table = nullptr;
};

#endif
