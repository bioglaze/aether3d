#include "AudioSourceComponent.hpp"
#include "AudioSystem.hpp"
#include <string>

static constexpr int MaxComponents = 30;

ae3d::AudioSourceComponent audioSourceComponents[ MaxComponents ];
unsigned nextFreeAudioSourceComponent = 0;

unsigned ae3d::AudioSourceComponent::New()
{
    if (nextFreeAudioSourceComponent == MaxComponents - 1)
    {
        return nextFreeAudioSourceComponent;
    }
    
    return nextFreeAudioSourceComponent++;
}

ae3d::AudioSourceComponent* ae3d::AudioSourceComponent::Get( unsigned index )
{
    return &audioSourceComponents[ index ];
}

void ae3d::AudioSourceComponent::SetClipId( unsigned audioClipId )
{
    clipId = audioClipId;
}

void ae3d::AudioSourceComponent::Play() const
{
    if (isEnabled)
    {
        AudioSystem::Play( clipId, isLooping );
    }
}

std::string GetSerialized( ae3d::AudioSourceComponent* component )
{
    std::string str( "audiosource\n" );
    str += std::to_string( component->GetClipID() ) + "\n";
    str += "enabled " + std::to_string( (int)component->IsEnabled() ) + "\n\n\n";
    
    return str;
}

