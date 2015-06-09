#include "CommandManager.hpp"
#include "Command.hpp"

void CommandManager::Execute( std::shared_ptr< CommandBase > command )
{
    command->Execute();
    undoStack.push( command );
}

void CommandManager::Undo()
{
    if (undoStack.size() > 0)
    {
        undoStack.top()->Undo();
        undoStack.pop();
    }
}
