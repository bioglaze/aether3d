#include "AudioSystem.hpp"
#include <string>
#import <AVFoundation/AVFoundation.h>
#import <Foundation/NSURL.h>
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#include "Array.hpp"
#include "FileSystem.hpp"
#include "FileWatcher.hpp"
#include "System.hpp"

extern ae3d::FileWatcher fileWatcher;

struct ClipInfo
{
    AVAudioPCMBuffer* buffer = nil;
    std::string path;
    float lengthInSeconds = 0;
};

namespace AudioGlobal
{
    AVAudioEngine* engine = nullptr;
    AVAudioPlayerNode* player = nullptr;
    Array< ClipInfo > clips;
}

void LoadOgg( const ae3d::FileSystem::FileContentsData& clipData, ClipInfo& info )
{
    ae3d::System::Print( "AudioSystem: .ogg not implemented on macOS/iOS yet: %s\n", clipData.path.c_str() );
    return;
}

void AudioReload( const std::string& path )
{
    ae3d::System::Print( "AudioSystem: AudioReload not implemented on macOS/iOS\n" );
}

void ae3d::AudioSystem::Init()
{
    AudioGlobal::engine = [ [AVAudioEngine alloc] init];
    AudioGlobal::player = [ [AVAudioPlayerNode alloc] init];
    [AudioGlobal::engine attachNode:AudioGlobal::player];
    
    AVAudioMixerNode* mixer = AudioGlobal::engine.mainMixerNode;
    [AudioGlobal::engine connect:AudioGlobal::player to:mixer format:[mixer outputFormatForBus:0]];
    
    NSError* error;
    
    if (![AudioGlobal::engine startAndReturnError:&error])
    {
        ae3d::System::Print( "Could not start audio system!\n" );
    }
}

void ae3d::AudioSystem::Deinit()
{
}

unsigned ae3d::AudioSystem::GetClipIdForData( const FileSystem::FileContentsData& clipData )
{
    // Checks cache for an already loaded clip from the same path.
    for (unsigned i = 0; i < AudioGlobal::clips.count; ++i)
    {
        if (AudioGlobal::clips[ i ].path == clipData.path)
        {
            return i;
        }
    }
    
    if (!clipData.isLoaded)
    {
        System::Print( "AudioSystem: File data %s not loaded!\n", clipData.path.c_str() );
        return 0;
    }

    const unsigned clipId = AudioGlobal::clips.count + 1;

    ClipInfo info;
    info.path = clipData.path;
    AudioGlobal::clips.Add( info );

    NSString* path = [NSString stringWithUTF8String: clipData.path.c_str()];

    NSURL* url = [ NSURL fileURLWithPath:path];
    AVAudioFile* avFile = [ [AVAudioFile alloc] initForReading:url error:nil ];
    AVAudioFormat* format = avFile.processingFormat;
    AVAudioFrameCount capacity = (AVAudioFrameCount)avFile.length;
    AudioGlobal::clips[ clipId - 1 ].buffer = [ [AVAudioPCMBuffer alloc] initWithPCMFormat:format frameCapacity:capacity];
    [avFile readIntoBuffer:AudioGlobal::clips[ clipId - 1 ].buffer error:nil];

    fileWatcher.AddFile( clipData.path, AudioReload );

    return clipId;
}

float ae3d::AudioSystem::GetClipLengthForId( unsigned handle )
{
    return handle < AudioGlobal::clips.count ? AudioGlobal::clips[ handle ].lengthInSeconds : 1;
}

void ae3d::AudioSystem::Play( unsigned clipId, bool isLooping )
{
    if (clipId >= AudioGlobal::clips.count + 1)
    {
        return;
    }
    
    [AudioGlobal::player scheduleBuffer:AudioGlobal::clips[ clipId - 1 ].buffer completionHandler:nil];
    [AudioGlobal::player play];
}

void ae3d::AudioSystem::SetListenerPosition( float x, float y, float z )
{
    // TODO: Implement
}

void ae3d::AudioSystem::SetListenerOrientation( float forwardX, float forwardY, float forwardZ )
{
    // TODO: Implement
}
