#include "GameObject.hpp"

unsigned ae3d::GameObject::GetNextComponentIndex()
{
    return nextFreeComponentIndex++;
}

std::string ae3d::GameObject::GetSerialized() const
{
    std::string outSerialized = "gameobject ";
    outSerialized += name + "\n";

    return outSerialized;
}
