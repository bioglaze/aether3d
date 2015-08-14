#ifndef MODIFYTRANSFORMCOMMAND_HPP
#define MODIFYTRANSFORMCOMMAND_HPP

#include <list>
#include <vector>
#include "Command.hpp"
#include "Vec3.hpp"
#include "Quaternion.hpp"

class SceneWidget;

class ModifyTransformCommand : public CommandBase
{
public:
    ModifyTransformCommand( SceneWidget* sceneWidget, const ae3d::Vec3& newPosition, const ae3d::Quaternion& newRotation, float newScale );
    void Execute() override;
    void Undo() override;

private:
    SceneWidget* sceneWidget = nullptr;
    ae3d::Vec3 position;
    ae3d::Quaternion rotation;
    float scale;
    std::vector< ae3d::Vec3 > oldPositions;
    std::vector< ae3d::Quaternion > oldRotations;
    std::vector< float > oldScales;
    std::list< int > selectedGameObjects;
};

#endif

