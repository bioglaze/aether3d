#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

namespace ae3d
{
    /**
      GameObject is composed of components that define its behavior.
    */
    class GameObject
    {
    public:
        static const int InvalidComponentIndex = -1;

        template< class T > void AddComponent()
        {
            const int index = GetNextComponentIndex();
            
            if (index != InvalidComponentIndex)
            {
                components[index].handle = T::New();
                components[index].type = T::Type();
            }
        }

        // \return The first component of type T or null if there is no such component.
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

#pragma message("TODO [2015-03-16] constexpr when VS supports it (VS 2015 should)")
        static const int MaxComponents = 10;
        int nextFreeComponentIndex = 0;
        ComponentEntry components[ MaxComponents ];
    };
}
#endif
