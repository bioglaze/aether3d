#ifndef MODIFYSPRITERENDERERCOMMAND_HPP
#define MODIFYSPRITERENDERERCOMMAND_HPP

#include "Command.hpp"
#include <string>

namespace ae3d
{
    class SpriteRendererComponent;
}

class ModifySpriteRendererCommand : public CommandBase
{
public:
    ModifySpriteRendererCommand( ae3d::SpriteRendererComponent* spriteRenderer, const std::string& path,
                                 float x, float y, float width, float height );
    void Execute() override;
    void Undo() override;

    ae3d::SpriteRendererComponent* spriteRenderer = nullptr;
    std::string path;
    float x, y;
    float width, height;

    std::string oldPath;
    float oldX, oldY;
    float oldWidth, oldHeight;
};

#endif
