#pragma once

namespace ae3d
{
    class GameObject;
}

class Inspector
{
  public:
    void Init();
    void BeginInput();
    void EndInput();
    void Deinit();
    void HandleLeftMouseClick( int x, int y, int state );
    void HandleMouseMotion( int x, int y );
    void Render( int width, int height );
    void SetGameObject( ae3d::GameObject* go ) { gameObject = go; }
    
private:
    ae3d::GameObject* gameObject = nullptr;
};
