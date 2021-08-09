#pragma once

namespace ae3d
{
    class ParticleSystemComponent
    {
    public:
        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }

        int GetMaxParticles() const { return maxParticles; }
        void SetMaxParticles( int count ) { maxParticles = count; }
        
    private:
        friend class GameObject;

        GameObject* gameObject = nullptr;
        int maxParticles = 100;
    };
}
