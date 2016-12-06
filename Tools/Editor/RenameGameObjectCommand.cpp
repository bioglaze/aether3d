#include "RenameGameObjectCommand.hpp"
#include "GameObject.hpp"
#include "System.hpp"

RenameGameObjectCommand::RenameGameObjectCommand( ae3d::GameObject* aGameObject, const std::string& aNewName )
{
    ae3d::System::Assert( aGameObject != nullptr, "RenameGameObjectCommand needs a game object" );

    gameObject = aGameObject;
    oldName = gameObject->GetName();
    newName = aNewName;
}

void RenameGameObjectCommand::Execute()
{
    gameObject->SetName( newName.c_str() );
}

void RenameGameObjectCommand::Undo()
{
    gameObject->SetName( oldName.c_str() );
}
