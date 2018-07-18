#pragma once

namespace ae3d
{
    class GameObject;
}

class Inspector
{
  public:
    void Init();
    void Deinit();
    void SetGameObject( ae3d::GameObject* go ) { gameObject = go; }
    void Render( int width, int height );
    
private:
    ae3d::GameObject* gameObject = nullptr;
};
