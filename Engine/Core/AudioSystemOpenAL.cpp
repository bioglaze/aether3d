#include "AudioSystem.hpp"
#include <sstream>
#include <vector>
#include <string>
#if defined __APPLE__
#    include <OpenAL/al.h>
#    include <OpenAL/alc.h>
#else
#    include "AL/al.h"
#    include "AL/alc.h"
#endif
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#include "System.hpp"

struct ClipInfo
{
    ALuint bufID;
    ALuint srcID;
};

namespace AudioGlobal
{
    ALCdevice* device = nullptr;
    ALCcontext* context = nullptr;
    std::vector< ClipInfo > clips;
}

/**
 * WAVE structure which is used by .wav files.
 * Follows the specification at the following URI:
 * <http://ccrma.stanford.edu/courses/422/projects/WaveFormat/>
 *
 * Assumes that unsigned int is 4 bytes and unsigned short is 2 bytes.
 */
struct WAVE
{
    unsigned char chunkID[ 4 ];
    unsigned chunkSize;
    unsigned char format[4];
    
    // Format sub chunk
    unsigned char subchunk1ID[ 4 ];
    unsigned subchunk1Size;
    unsigned short audioFormat;
    unsigned short numChannels;
    unsigned sampleRate;
    unsigned byteRate;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
    
    // Data sub chunk
    unsigned char subchunk2ID[ 4 ]; // Data sub chunk ID.
    unsigned subchunk2Size; // Data sub chunk size.
    std::vector< unsigned char > data; // Chunk data.
};

static const char * GetOpenALErrorString( int errID )
{
    if (errID == AL_NO_ERROR) return "";
    if (errID == AL_INVALID_NAME) return "Invalid name";
    if (errID == AL_INVALID_ENUM) return " Invalid enum ";
    if (errID == AL_INVALID_VALUE) return " Invalid value ";
    if (errID == AL_INVALID_OPERATION) return " Invalid operation ";
    if (errID == AL_OUT_OF_MEMORY) return " Out of memory! ";
    
    return " Don't know ";
}

static void CheckOpenALError( const char* info )
{
    const ALenum err = alGetError();
    
    if (err != AL_NO_ERROR)
    {
        ae3d::System::Print( "OpenAL error (%s) for %s\n", GetOpenALErrorString( err ), info );
    }
}

static void LoadOgg( const ae3d::System::FileContentsData& clipData )
{
    short* decoded = nullptr;
    int channels = 0;
    int samplerate = 0;
    const int len = stb_vorbis_decode_memory( clipData.data.data(), static_cast< int >( clipData.data.size() ), &channels, &samplerate, &decoded );
    
    if (len == 0)
    {
        ae3d::System::Print( "AudioSystem: Could not open %s\n", clipData.path.c_str() );
        return;
    }
    
    ClipInfo info;
    alGenBuffers( 1, &info.bufID );
    alGenSources( 1, &info.srcID );

    alListener3f( AL_POSITION, 0.0f, 0.0f, 0.0f );
    alSource3f( info.srcID, AL_POSITION, 0.0f, 0.0f, 0.0f );
    
    int error;
    stb_vorbis* vorbis = stb_vorbis_open_memory( clipData.data.data(), static_cast< int >( clipData.data.size() ), &error, nullptr );
    const stb_vorbis_info vinfo = stb_vorbis_get_info( vorbis );
    
    const ALenum format = vinfo.channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
    
    alBufferData( info.bufID, format, &decoded[0], len, vinfo.sample_rate );
    alSourcei( info.srcID, AL_BUFFER, info.bufID );
    
    CheckOpenALError("Loading ogg");
    AudioGlobal::clips.push_back( info );
}

static void LoadWav( const ae3d::System::FileContentsData& clipData )
{
    std::istringstream ifs( std::string( clipData.data.begin(), clipData.data.end() ) );

    WAVE wav;
    
    ifs.read( (char*)&wav.chunkID, 4 );
    if (wav.chunkID[0] != 'R' && wav.chunkID[1] != 'I' &&
        wav.chunkID[2] != 'F' && wav.chunkID[3] != 'F')
    {
        ae3d::System::Print( "LoadWav: %s is not a valid .wav file!\n", clipData.path.c_str() );
        return;
    }
    
    ifs.read( (char*)&wav.chunkSize, 4 );
    
    ifs.read( (char*)&wav.format, 4 );
    if (wav.format[0] != 'W' && wav.format[1] != 'A' &&
        wav.format[2] != 'V' && wav.format[3] != 'E')
    {
        ae3d::System::Print( "%s is not a valid .wav file!\n", clipData.path.c_str() );
        return;
    }
    
    ifs.read( (char*)&wav.subchunk1ID, 4 );
    if (wav.subchunk1ID[0] != 'f' && wav.subchunk1ID[1] != 'm' &&
        wav.subchunk1ID[2] != 't' && wav.subchunk1ID[3] != ' ')
    {
        ae3d::System::Print( "%s is not a valid .wav file!\n", clipData.path.c_str() );
        return;
    }
    
    ifs.read( (char*)&wav.subchunk1Size, 4 );
    if (wav.subchunk1Size != 16)
    {
        ae3d::System::Print( "%s does not contain PCM audio data!\n", clipData.path.c_str() );
        return;
    }
    
    ifs.read( (char*)&wav.audioFormat, 2 );
    if (wav.audioFormat != 1)
    {
        ae3d::System::Print( "%s does not contain uncompressed PCM audio data!\n", clipData.path.c_str() );
        return;
    }
    
    ifs.read( (char*)&wav.numChannels,   2 );
    ifs.read( (char*)&wav.sampleRate,    4 );
    ifs.read( (char*)&wav.byteRate,      4 );
    ifs.read( (char*)&wav.blockAlign,    2 );
    ifs.read( (char*)&wav.bitsPerSample, 2 );
    
    ifs.read( (char*)&wav.subchunk2ID, 4 );
    
    if (wav.subchunk2ID[0] != 'd' && wav.subchunk2ID[1] != 'a' &&
        wav.subchunk2ID[2] != 't' && wav.subchunk2ID[3] != 'a')
    {
        ae3d::System::Print( "%s does not contain data chunk!\n", clipData.path.c_str() );
        return;
    }
    
    ifs.read( (char*)&wav.subchunk2Size, 4 );
    
    // Data size is file size minus header size.
    // We get it by saving the current position, moving
    // to the EOF, checking EOF's position and moving back to
    // the saved location for data reading.
    const std::streampos dataPos = ifs.tellg();
    ifs.seekg( 0, std::ios::end );
    const std::streampos fileSize = ifs.tellg();
    const ALsizei dataSize = (ALsizei)fileSize - 44; // Header is 44 bytes.
    
    ifs.seekg( dataPos );
    
    wav.data.resize( dataSize );
    
    ifs.read( (char*)wav.data.data(), dataSize );
    
    ALenum format = AL_FORMAT_MONO8;
    
    if (wav.numChannels == 1 && wav.bitsPerSample == 8)
    {
        format = AL_FORMAT_MONO8;
    }
    else if (wav.numChannels == 2 && wav.bitsPerSample == 8)
    {
        format = AL_FORMAT_STEREO8;
    }
    else if (wav.numChannels == 1 && wav.bitsPerSample == 16)
    {
        format = AL_FORMAT_MONO16;
    }
    else if (wav.numChannels == 2 && wav.bitsPerSample == 16)
    {
        format = AL_FORMAT_STEREO16;
    }
    else
    {
        ae3d::System::Print( "Audio: Unknown format in file %s\n", clipData.path.c_str() );
    }
    
    ClipInfo info;
    alGenBuffers( 1, &info.bufID );
    alGenSources( 1, &info.srcID );
    
    alBufferData( info.bufID, format, wav.data.data(), dataSize, wav.sampleRate );
    alSourcei( info.srcID, AL_BUFFER, info.bufID );
    alSourcef( info.srcID, AL_GAIN, 1.0f );
    
    CheckOpenALError( "Loading .wav data." );
    AudioGlobal::clips.push_back( info );
}

void ae3d::AudioSystem::Init()
{
    AudioGlobal::device = alcOpenDevice( nullptr );
    AudioGlobal::context = alcCreateContext( AudioGlobal::device, nullptr );
    alcMakeContextCurrent( AudioGlobal::context );
    
    if (!AudioGlobal::context)
    {
        ae3d::System::Print( "Error creating audio context!\n" );
    }
    
    alListener3f( AL_POSITION, 0, 0, 1 );
    CheckOpenALError( "AudioImpl::Init" );
}

void ae3d::AudioSystem::Deinit()
{
    for (const auto& it : AudioGlobal::clips)
    {
        alSourceStopv( 1, &it.srcID );
        alDeleteSources( 1, &it.srcID );
        alDeleteBuffers( 1, &it.bufID );
    }
    
    alcMakeContextCurrent( nullptr );
    
    if (AudioGlobal::context != nullptr)
    {
        alcDestroyContext( AudioGlobal::context );
    }
    
    if (AudioGlobal::device != nullptr)
    {
        alcCloseDevice( AudioGlobal::device );
    }
}

unsigned ae3d::AudioSystem::GetClipIdForData( const System::FileContentsData& clipData )
{
    if (!clipData.isLoaded)
    {
        ae3d::System::Print( "AudioSystem: File data %s not loaded!", clipData.path.c_str() );
        return 0;
    }

    unsigned clipId = static_cast< unsigned >( AudioGlobal::clips.size() );
    
    const std::string extension = clipData.path.substr( clipData.path.length() - 3, clipData.path.length() );
    
    if (extension == "wav" || extension == "WAV")
    {
        LoadWav( clipData );
    }
    else if (extension == "ogg" || extension == "OGG")
    {
        LoadOgg( clipData );
    }
    else
    {
        System::Print( "Unsupported audio file extension in %d. Must be .wav or .ogg.\n", clipData.path.c_str() );
    }
    
    return clipId;
}

void ae3d::AudioSystem::Play( unsigned clipId )
{
    if (clipId >= AudioGlobal::clips.size())
    {
        return;
    }
    
    ALint state;
    alGetSourcei( AudioGlobal::clips[ clipId ].srcID, AL_SOURCE_STATE, &state );
    
    if (state != AL_PLAYING)
    {
        alSourcePlay( AudioGlobal::clips[ clipId ].srcID );
    }
}
