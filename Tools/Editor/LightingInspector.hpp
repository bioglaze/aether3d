#ifndef LIGHTINGINSPECTOR_HPP
#define LIGHTINGINSPECTOR_HPP

#include <string>
#include <QObject>
#include "TextureCube.hpp"

namespace ae3d
{
    class GameObject;
}

class LightingInspector : public QObject
{
    Q_OBJECT

  public:
    class QWidget* GetWidget() { return root; }
    void Init( class SceneWidget* sceneWidget );

private slots:
    void SkyboxCellClicked( int, int );

private:
    QWidget* root = nullptr;
    SceneWidget* sceneWidget = nullptr;
    class QTableWidget* skyboxTable = nullptr;
    std::string skyboxTexturePaths[ 6 ];
    ae3d::TextureCube skyboxCube;
};

#endif
