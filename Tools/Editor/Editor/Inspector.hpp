#pragma once

namespace ae3d
{
    class GameObject;
    class Material;
}

class Inspector
{
  public:
    enum class Command { Empty, CreateGO, OpenScene, SaveScene };

    void Init();
    void BeginInput();
    void EndInput();
    void Deinit();
    void HandleLeftMouseClick( int x, int y, int state );
    void HandleMouseMotion( int x, int y );
    void Render( unsigned width, unsigned height, ae3d::GameObject* gameObject, Command& outCommand, ae3d::GameObject** gameObjects, unsigned goCount, ae3d::Material* material );
};
