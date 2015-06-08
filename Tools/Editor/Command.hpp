#ifndef COMMAND
#define COMMAND

class ICommand
{
public:
    virtual void Execute() = 0;
    virtual void Undo() = 0;
};

#endif

