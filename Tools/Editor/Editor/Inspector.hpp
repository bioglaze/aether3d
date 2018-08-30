#pragma once

namespace ae3d
{
    class GameObject;
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
    void Render( int width, int height, ae3d::GameObject* gameObject, Command& outCommand, ae3d::GameObject** gameObjects, int goCount );
};
