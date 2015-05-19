#ifndef AUDIO_CLIP_H
#define AUDIO_CLIP_H

namespace ae3d
{
    namespace FileSystem
    {
        struct FileContentsData;
    }

    /// Audio clip.
    class AudioClip
    {
      public:
        /** Loads a clip from .wav or Ogg Vorbis file data. */
        void Load( const FileSystem::FileContentsData& clipData );

        /** \return Clip's handle. */
        unsigned GetId() const { return id; }

      private:
        unsigned id = 0;
    };
}

#endif
