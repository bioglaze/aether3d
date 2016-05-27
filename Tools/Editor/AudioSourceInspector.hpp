#ifndef AUDIOSOURCEINSPECTOR_HPP
#define AUDIOSOURCEINSPECTOR_HPP

#include <list>
#include <QObject>

namespace ae3d
{
    class GameObject;
}

class AudioSourceInspector : public QObject
{
    Q_OBJECT

  public:
    class QWidget* GetWidget() { return root; }
    void Init( QWidget* mainWindow );

private slots:
    void AudioCellClicked( int, int );
    void GameObjectSelected( std::list< ae3d::GameObject* > );

  private:
    QWidget* root = nullptr;
    class QTableWidget* audioClipTable = nullptr;
    class QPushButton* removeButton = nullptr;
};

#endif
