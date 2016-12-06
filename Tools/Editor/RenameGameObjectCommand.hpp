#ifndef RENAMEGAMEOBJECTCOMMAND_H
#define RENAMEGAMEOBJECTCOMMAND_H

#include "Command.hpp"
#include <string>

namespace ae3d
{
    class GameObject;
}

class RenameGameObjectCommand : public CommandBase
{
public:
    explicit RenameGameObjectCommand( ae3d::GameObject* gameObject, const std::string& newName );
    void Execute() override;
    void Undo() override;

private:
    std::string oldName;
    std::string newName;
    ae3d::GameObject* gameObject = nullptr;
};

#endif
