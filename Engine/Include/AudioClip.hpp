#ifndef AUDIO_CLIP_H
#define AUDIO_CLIP_H

namespace ae3d
{
    namespace System
    {
        struct FileContentsData;
    }

    /* Audio clip. */
    class AudioClip
    {
      public:
        /* Loads a clip from .wav or Ogg Vorbis file data. */
        void Load( const System::FileContentsData& clipData );

      private:
        friend class AudioSourceComponent;

        /* \return Clip's handle. */
        unsigned GetId() const { return id; }
        
        unsigned id = 0;
    };
}

#endif
