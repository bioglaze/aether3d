#ifndef COMMANDMANAGER_HPP
#define COMMANDMANAGER_HPP

#include <stack>
#include <memory>

class ICommand;

class CommandManager
{
public:
    CommandManager();
    void Execute( std::shared_ptr< ICommand > command );
    void Undo();

private:
    std::stack< std::shared_ptr< ICommand > > undoStack;
};

#endif
