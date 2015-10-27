#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

namespace ae3d
{
    namespace FileSystem
    {
        struct FileContentsData;
    }

    namespace AudioSystem
    {
        /// Creates the audio device. Must be called before other methods in this namespace.
        void Init();
        
        /// Releases all allocated handles and shuts down the audio system.
        void Deinit();
        
        /*
          Loads a clip data and returns its handle that can be used to play the clip.
         
          \param clipData .wav or Ogg Vorbis audio data.
          \return Clip handle that can be passed to Play.
         */
        unsigned GetClipIdForData( const FileSystem::FileContentsData& clipData );
        
        /// \return Length in seconds.
        float GetClipLengthForId( unsigned handle );
        
        /// \param clipId Clip handle from GetClipIdForData.
        void Play( unsigned clipId );
    }
}

#endif
