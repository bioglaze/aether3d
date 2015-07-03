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

        /// \return The first component of type T or null if there is no such component.
        template< class T > T* GetComponent()
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

        /// \param aName Game Object's name.
        void SetName( const char* aName ) { name = aName; }
        
        /// \return Game Object's name.
        const std::string& GetName() const { return name; }
        
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
    };
}
#endif
