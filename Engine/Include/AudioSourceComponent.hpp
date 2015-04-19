#ifndef AUDIO_SOURCE_COMPONENT
#define AUDIO_SOURCE_COMPONENT

namespace ae3d
{
    class AudioClip;
    
    /** Contains an audio clip and methods to play it. */
    class AudioSourceComponent
    {
    public:
        /** \param audioClip Audio clip */
        void SetClip( AudioClip* audioClip );

        /** Plays the clip. */
        void Play();

    private:
        friend class GameObject;
        
        /** \return Component's type code. Must be unique for each component type. */
        static int Type() { return 3; }
        
        /** \return Component handle that uniquely identifies the instance. */
        static int New();
        
        /** \return Component at index or null if index is invalid. */
        static AudioSourceComponent* Get(int index);
        
        AudioClip* clip = nullptr;
    };
}

#endif
