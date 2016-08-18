#ifndef COMMANDMANAGER_HPP
#define COMMANDMANAGER_HPP

#include <stack>
#include <memory>

class CommandBase;

class CommandManager
{
public:
    void Execute( std::shared_ptr< CommandBase > command );
    void Undo();
    bool HasUnsavedChanges() const { return hasUnsavedChanges; }
    void SceneSaved() { hasUnsavedChanges = false; }

private:
    std::stack< std::shared_ptr< CommandBase > > undoStack;
    bool hasUnsavedChanges = false;
};

#endif
