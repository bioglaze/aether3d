#ifndef COMMAND
#define COMMAND

/// Everything that modifies the scene is a command. This allows undo.
class CommandBase
{
public:
    virtual void Execute() = 0;
    virtual void Undo() = 0;
};

#endif

