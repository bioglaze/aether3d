#include "AudioClip.hpp"
#include "AudioSystem.hpp"

void ae3d::AudioClip::Load( const FileSystem::FileContentsData& clipData )
{
    handle = AudioSystem::GetClipIdForData( clipData );
    length = AudioSystem::GetClipLengthForId( handle );
}
