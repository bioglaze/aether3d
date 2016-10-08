#include "ModifySpriteRendererCommand.hpp"
#include "SpriteRendererComponent.hpp"
#include "System.hpp"

ModifySpriteRendererCommand::ModifySpriteRendererCommand( ae3d::SpriteRendererComponent* aSpriteRenderer, const std::string& aPath,
                                                          float aX, float aY, float aWidth, float aHeight )
    : spriteRenderer( aSpriteRenderer )
    , path( aPath )
    , x( aX )
    , y( aY )
    , width( aWidth )
    , height( aHeight )
{
    ae3d::System::Assert( spriteRenderer != nullptr, "ModifySpriteRenderer command needs SpriteRenderer!" );

    auto info = aSpriteRenderer->GetSpriteInfo( 0 );
    oldPath = info.path;
    oldX = info.x;
    oldY = info.y;
    oldWidth = info.width;
    oldHeight = info.height;
}

void ModifySpriteRendererCommand::Execute()
{
    ae3d::System::Assert( spriteRenderer != nullptr, "ModifySpriteRenderer command needs SpriteRenderer!" );
    //spriteRenderer->SetTexture();
}

void ModifySpriteRendererCommand::Undo()
{

}
