#include "CommandManager.hpp"
#include "Command.hpp"

void CommandManager::Execute( std::shared_ptr< CommandBase > command )
{
    command->Execute();
    undoStack.push( command );
    hasUnsavedChanges = true;
}

void CommandManager::Undo()
{
    if (!undoStack.empty())
    {
        undoStack.top()->Undo();
        undoStack.pop();
    }
}
