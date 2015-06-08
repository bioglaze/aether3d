#include "CommandManager.hpp"
#include "Command.hpp"

CommandManager::CommandManager()
{

}

void CommandManager::Execute( std::shared_ptr< ICommand > command )
{
    command->Execute();
    undoStack.push( command );
}

void CommandManager::Undo()
{

}
