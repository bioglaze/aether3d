#include "GameObject.hpp"

unsigned ae3d::GameObject::GetNextComponentIndex()
{
    return nextFreeComponentIndex >= MaxComponents ? InvalidComponentIndex : nextFreeComponentIndex++;
}

std::string ae3d::GameObject::GetSerialized() const
{
    std::string outSerialized = "gameobject ";
    outSerialized += name + "\n";

    return outSerialized;
}
