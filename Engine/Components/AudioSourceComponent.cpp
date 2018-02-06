#include "AudioSourceComponent.hpp"
#include "AudioSystem.hpp"
#include <vector>
#include <sstream>
#include <string>

std::vector< ae3d::AudioSourceComponent > audioSourceComponents;
unsigned nextFreeAudioSourceComponent = 0;

unsigned ae3d::AudioSourceComponent::New()
{
    if (nextFreeAudioSourceComponent == audioSourceComponents.size())
    {
        audioSourceComponents.resize( audioSourceComponents.size() + 10 );
    }
    
    return nextFreeAudioSourceComponent++;
}

ae3d::AudioSourceComponent* ae3d::AudioSourceComponent::Get( unsigned index )
{
    return &audioSourceComponents[index];
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
    std::stringstream outStream;
    outStream << "audiosource\n";
    outStream << component->GetClipID() << "\n";
    outStream << "enabled " << component->IsEnabled() << "\n";
    outStream << "\n\n";
    
    return outStream.str();
}

