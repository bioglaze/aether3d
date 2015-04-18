#include "AudioSourceComponent.hpp"
#include "AudioClip.hpp"
#include "AudioSystem.hpp"
#include <vector>

std::vector< ae3d::AudioSourceComponent > audioSourceComponents;
int nextFreeAudioSourceComponent = 0;

int ae3d::AudioSourceComponent::New()
{
    if (nextFreeAudioSourceComponent == audioSourceComponents.size())
    {
        audioSourceComponents.resize( audioSourceComponents.size() + 10 );
    }
    
    return nextFreeAudioSourceComponent++;
}

ae3d::AudioSourceComponent* ae3d::AudioSourceComponent::Get( int index )
{
    return &audioSourceComponents[index];
}

void ae3d::AudioSourceComponent::SetClip( AudioClip* audioClip )
{
    clip = audioClip;
}

void ae3d::AudioSourceComponent::Play()
{
    AudioSystem::Play( clip->GetId() );
}
