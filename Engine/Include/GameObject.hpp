#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

namespace ae3d
{
    /* GameObject is composed of components that define its behavior. */
    class GameObject
    {
    public:
        /* Invalid component index. */
        static const int InvalidComponentIndex = -1;

        /* Adds a component into the game object. There can be multiple components of the same type. */
        template< class T > void AddComponent()
        {
            const int index = GetNextComponentIndex();
            
            if (index != InvalidComponentIndex)
            {
                components[index].handle = T::New();
                components[index].type = T::Type();
            }
        }

        /* \return The first component of type T or null if there is no such component. */
        template< class T > T* GetComponent()
        {
            for (const auto& component : components)
            {
                if (T::Type() == component.type)
                {
                    return T::Get(component.handle);
                }
            }

            return nullptr;
        }

    private:
        struct ComponentEntry
        {
            int type = 0;
            int handle = 0;
        };

        int GetNextComponentIndex();

        static const int MaxComponents = 10;
        int nextFreeComponentIndex = 0;
        ComponentEntry components[ MaxComponents ];
    };
}
#endif
