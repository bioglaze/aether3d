#pragma once
#include "FileSystem.hpp"
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
}

struct TransformGizmo
{
    void Init( ae3d::Shader* shader );
    void SetPosition( const ae3d::Vec3& position );
    
    ae3d::Mesh mesh;
    
    ae3d::GameObject go;
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
    }
    
    T& operator=( const T& other )
    {
        delete[] elements;
        elementCount = other.elementCount;
        elements = new T[ elementCount ];
        
        for (std::size_t index = 0; index < elementCount; ++index)
        {
            elements[ index ] = other.elements[ index ];
        }
    }
    
    T& operator[]( std::size_t index ) const
    {
        return elements[ index ];
    }
    
    std::size_t GetLength() const
    {
        return elementCount;
    }
    
    void PushBack( const T& item )
    {
        T* after = new T[ elementCount + 1 ];
        
        std::memcpy( after, elements, elementCount * sizeof( T ) );
        
        after[ elementCount ] = item;
        delete[] elements;
        elements = after;
        elementCount = elementCount + 1;
    }
    
    void Allocate( std::size_t size )
    {
        elements = new T[ size ];
        elementCount = size;
    }
    
    private:
    T* elements = nullptr;
    std::size_t elementCount = 0;
};

class SceneView
{
public:
    void Init( int width, int height );
    void BeginRender();
    void EndRender();
    void RotateCamera( float xDegrees, float yDegrees );
    void MoveCamera( const ae3d::Vec3& moveDir );
    void SelectObject( int screenX, int screenY, int width, int height );

private:
    Array< ae3d::GameObject > gameObjects;
    ae3d::GameObject camera;
    ae3d::Scene scene;
    ae3d::Shader unlitShader;
    TransformGizmo transformGizmo;
    
    // TODO: Test content, remove when stuff works.
    ae3d::Texture2D gliderTex;
    ae3d::Material material;
    ae3d::Mesh cubeMesh;
};
