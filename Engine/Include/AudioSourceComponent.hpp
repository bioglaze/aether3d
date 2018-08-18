#pragma once

namespace ae3d
{
    /// Contains an audio clip and methods to play it. \see AudioClip
    class AudioSourceComponent
    {
    public:
        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }

        /// \param enabled True if the component should be audible, false otherwise.
        void SetEnabled( bool enabled ) { isEnabled = enabled; }

        /// \return True, if clips played through this source will be affected by this game object's and camera's position.
        bool Is3D() const { return is3D; }

        /// \return True, if enabled
        bool IsEnabled() const { return isEnabled; }
        
        /// \return Clip's id
        unsigned GetClipID() const { return clipId; }
        
        /// \param audioClipId Audio clip id.
        void SetClipId( unsigned audioClipId );

        /// \param enable True, if clips played through this source will be affected by this GameObject's and camera's position.
        void Set3D( bool enable ) { is3D = enable; }
        
        /// \param enable True, if the clip will be looped when playing.
        void SetLooping( bool enable ) { isLooping = enable; }
        
        /// Plays the clip.
        void Play() const;

    private:
        friend class GameObject;
        
        /// \return Component's type code. Must be unique for each component type.
        static int Type() { return 3; }
        
        /// \return Component handle that uniquely identifies the instance.
        static unsigned New();
        
        /// \return Component at index or null if index is invalid.
        static AudioSourceComponent* Get( unsigned index );
        
        GameObject* gameObject = nullptr;
        unsigned clipId = 0;
        bool is3D = false;
        bool isLooping = false;
        bool isEnabled = true;
    };
}
