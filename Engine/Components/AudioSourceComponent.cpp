#include "AudioSourceComponent.hpp"
#include "AudioSystem.hpp"
#include <vector>
#include <sstream>

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

void ae3d::AudioSourceComponent::Play()
{
    AudioSystem::Play( clipId );
}

std::string ae3d::AudioSourceComponent::GetSerialized() const
{
    std::stringstream outStream;
    outStream << "audiosource\n" << clipId << "\n\n";
    return outStream.str();
}

