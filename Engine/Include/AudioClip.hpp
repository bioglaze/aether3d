#pragma once

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
        /// \param clipData Clip data from .wav or .ogg file.
        void Load( const FileSystem::FileContentsData& clipData );

        /// \return Clip's handle. 0 means that the clip is empty/not pointing to any audio clip.
        unsigned GetId() const { return handle; }

        /// \return Clip's length in seconds or 1 if the clip is not loaded.
        float LengthInSeconds() const { return length; }
        
      private:
        unsigned handle = 0;
        float length = 0;
    };
}

