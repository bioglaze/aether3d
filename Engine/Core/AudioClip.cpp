#include "AudioClip.hpp"
#include "AudioSystem.hpp"
#include "System.hpp"

void ae3d::AudioClip::Load( const System::FileContentsData& clipData )
{
    id = AudioSystem::GetClipIdForData( clipData );
}
