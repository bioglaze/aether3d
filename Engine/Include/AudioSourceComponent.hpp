#ifndef AUDIO_SOURCE_COMPONENT
#define AUDIO_SOURCE_COMPONENT

#include <string>

namespace ae3d
{
    /// Contains an audio clip and methods to play it.
    class AudioSourceComponent
    {
    public:
        /// \param audioClipId Audio clip id.
        void SetClipId( unsigned audioClipId );

        /// Plays the clip.
        void Play() const;

        /// \return Textual representation of component.
        std::string GetSerialized() const;

    private:
        friend class GameObject;
        
        /// \return Component's type code. Must be unique for each component type.
        static int Type() { return 3; }
        
        /// \return Component handle that uniquely identifies the instance.
        static unsigned New();
        
        /// \return Component at index or null if index is invalid.
        static AudioSourceComponent* Get( unsigned index );
        
        unsigned clipId = 0;
    };
}

#endif
