#pragma once
#include "GameObject.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "Shader.hpp"
#include "Texture2D.hpp"
#include <cstring>

namespace ae3d
{
    struct Vec3;
    class GameObject;

    namespace FileSystem
    {
        struct FileContentsData;
    }
}

struct TransformGizmo
{
    void Init( ae3d::Shader* shader, ae3d::GameObject& go );
    
    ae3d::Mesh mesh;
    
    ae3d::Mesh translateMesh;
    ae3d::Material xAxisMaterial;
    ae3d::Material yAxisMaterial;
    ae3d::Material zAxisMaterial;
};

template< typename T > class Array
{
  public:
    ~Array()
    {
        delete[] elements;
        elements = nullptr;
        elementCount = 0;
    }
    
    T& operator=( const T& other )
    {
        delete[] elements;
        elementCount = other.elementCount;
        elements = new T[ elementCount ];
        std::memcpy( elements, other.elements, elementCount * sizeof( T ) );
    }
    
    T& operator[]( int index ) const
    {
        return elements[ index ];
    }
    
    int GetLength() const noexcept
    {
        return elementCount;
    }
    
    void Add( const T& item )
    {
        T* after = new T[ elementCount + 1 ];
        std::memcpy( after, elements, elementCount * sizeof( T ) );
        
        after[ elementCount ] = item;
        delete[] elements;
        elements = after;
        elementCount = elementCount + 1;
    }

    void Allocate( int size )
    {
        delete[] elements;
        elements = size > 0 ? new T[ size ] : nullptr;
        elementCount = size;
    }
    
  private:
    T* elements = nullptr;
    int elementCount = 0;
};

class SceneView
{
public:
    void Init( int width, int height );
    void BeginRender();
    void EndRender();
    void LoadScene( const ae3d::FileSystem::FileContentsData& contents );
    void RotateCamera( float xDegrees, float yDegrees );
    void MoveCamera( const ae3d::Vec3& moveDir );
    void MoveSelection( const ae3d::Vec3& moveDir );
    ae3d::GameObject* SelectObject( int screenX, int screenY, int width, int height );

private:
    Array< ae3d::GameObject* > gameObjects;
    Array< ae3d::GameObject* > selectedGameObjects;
    ae3d::GameObject camera;
    ae3d::Scene scene;
    ae3d::Shader unlitShader;
    TransformGizmo transformGizmo;
    
    // TODO: Test content, remove when stuff works.
    ae3d::Texture2D gliderTex;
    ae3d::Material material;
    ae3d::Mesh cubeMesh;
};
