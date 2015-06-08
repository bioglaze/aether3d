#ifndef CREATECAMERACOMMAND_HPP
#define CREATECAMERACOMMAND_HPP

#include "Command.hpp"

class CreateCameraCommand : public ICommand
{
public:
    void Execute() override;
    void Undo() override;
};

#endif
