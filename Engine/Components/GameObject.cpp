#include "GameObject.hpp"

unsigned ae3d::GameObject::GetNextComponentIndex()
{
    return nextFreeComponentIndex++;
}
