#include "AudioClip.hpp"
#include "AudioSystem.hpp"
#include "FileSystem.hpp"

void ae3d::AudioClip::Load( const FileSystem::FileContentsData& clipData )
{
    id = AudioSystem::GetClipIdForData( clipData );
}
