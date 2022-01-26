#pragma once

namespace ae3d
{
    class ParticleSystemComponent
    {
    public:
        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }

        int GetMaxParticles() const { return maxParticles; }
        void SetMaxParticles( int count );

        void GetColor( float& outR, float& outG, float& outB )
        {
            outR = red;
            outG = green;
            outB = blue;
        }
        
        /// \param r Red in range 0-1.
        /// \param g Green in range 0-1.
        /// \param b Blue in range 0-1.
        void SetColor( float r, float g, float b )
        {
            red = r;
            green = g;
            blue = b;
        }
        
        void Simulate( class ComputeShader& simulationShader );
        void Cull( class ComputeShader& cullShader );

        void Draw( ComputeShader& drawShader, class RenderTexture& target );
        
    private:
        friend class GameObject;

        /** \return Component's type code. Must be unique for each component type. */
        static int Type() { return 11; }
        
        /** \return Component handle that uniquely identifies the instance. */
        static unsigned New();
        
        /** \return Component at index or null if index is invalid. */
        static ParticleSystemComponent* Get( unsigned index );

        static bool IsAnyAlive();

        GameObject* gameObject = nullptr;
        int maxParticles = 1000;
        float red = 1.0f;
        float green = 1.0f;
        float blue = 1.0f;
    };
}
