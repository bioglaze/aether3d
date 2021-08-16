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

        static void Simulate();
        
    private:
        friend class GameObject;

        /** \return Component's type code. Must be unique for each component type. */
        static int Type() { return 11; }
        
        /** \return Component handle that uniquely identifies the instance. */
        static unsigned New();
        
        /** \return Component at index or null if index is invalid. */
        static ParticleSystemComponent* Get( unsigned index );

        GameObject* gameObject = nullptr;
        int maxParticles = 100;
    };
}
