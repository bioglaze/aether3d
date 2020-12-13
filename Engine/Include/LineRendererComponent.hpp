#pragma once

namespace ae3d
{
    /// Renders lines. Use this component to get proper depth testing relative to other objects in a scene.
    /// For debug visualization lines you can use System::DrawLines() which is simpler.
    class LineRendererComponent
    {
    public:
        /// Constructor.
        LineRendererComponent();

        /// \return GameObject that owns this component.
        class GameObject* GetGameObject() const { return gameObject; }

        /// \param enabled True if the component should be rendered, false otherwise.
        void SetEnabled( bool enabled ) { isEnabled = enabled; }

        /// \return Line handle.
        int GetLineHandle() const { return lineHandle; }

        /// \param handle Line handle. Use System::CreateLineBuffer() to generate the handle.
        void SetLineHandle( int handle ) { lineHandle = handle; }
        
    private:
        friend class GameObject;
        friend class Scene;
        
        /* \return Component's type code. Must be unique for each component type. */
        static int Type() { return 10; }
        
        /* \return Component handle that uniquely identifies the instance. */
        static unsigned New();
        
        /* \return Component at index or null if index is invalid. */
        static LineRendererComponent* Get( unsigned index );
                
        GameObject* gameObject = nullptr;
        int lineHandle = 0;
        bool isEnabled = true;
    };
}
