#include "GameObject.hpp"

int ae3d::GameObject::GetNextComponentIndex()
{
    return nextFreeComponentIndex++;
}
