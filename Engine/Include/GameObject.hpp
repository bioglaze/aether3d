#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include <string>

namespace ae3d
{
    /// GameObject is composed of components that define its behavior.
    class GameObject
    {
    public:
        /// Invalid component index.
        static const unsigned InvalidComponentIndex = 99999999;

        /// Adds a component into the game object. There can be multiple components of the same type.
        template< class T > void AddComponent()
        {
            const unsigned index = GetNextComponentIndex();
            
            if (index != InvalidComponentIndex)
            {
                components[ index ].handle = T::New();
                components[ index ].type = T::Type();
            }
        }

        /// Remove a component from the game object.
        template< class T > void RemoveComponent()
        {
            for (unsigned i = 0; i < nextFreeComponentIndex; ++i)
            {
                if (components[ i ].type == T::Type())
                {
                    components[ i ].handle = 0;
                    components[ i ].type = -1;
                    return;
                }
            }
        }

        /// \return The first component of type T or null if there is no such component.
        template< class T > T* GetComponent() const
        {
            for (const auto& component : components)
            {
                if (T::Type() == component.type)
                {
                    return T::Get( component.handle );
                }
            }

            return nullptr;
        }

        /// Constructor.
        GameObject() = default;

        /// Copy constructor.
        GameObject( const GameObject& other );

        /// \param go Other game object.
        GameObject& operator=( const GameObject& go );

        /// \param aName Game Object's name.
        void SetName( const char* aName ) { name = aName; }
        
        /// \return Game Object's name.
        const std::string& GetName() const { return name; }
        
        /// \param aLayer Layer for controlling camera visibility etc. Must be power of two (2, 4, 8 etc.)
        void SetLayer( unsigned aLayer ) { layer = aLayer; }

        /// \return Layer.
        unsigned GetLayer() const { return layer; }

        /// \return Game Object's contents excluding components
        std::string GetSerialized() const;

    private:
        struct ComponentEntry
        {
            int type = -1;
            unsigned handle = 0;
        };

        unsigned GetNextComponentIndex();

        static const int MaxComponents = 10;
        unsigned nextFreeComponentIndex = 0;
        ComponentEntry components[ MaxComponents ];
        std::string name;
        unsigned layer = 1;
    };
}
#endif
