#pragma once
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "Shader.hpp"
#include "Texture2D.hpp"

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

class SceneView
{
public:
    void Init( int width, int height );
    void BeginRender();
    void EndRender();
    void RotateCamera( float xDegrees, float yDegrees );
    void MoveCamera( const ae3d::Vec3& moveDir );
    
private:
    ae3d::GameObject camera;
    ae3d::Scene scene;
    ae3d::Shader unlitShader;
    TransformGizmo transformGizmo;
    
    // TODO: Test content, remove when stuff works.
    ae3d::GameObject cube;
    ae3d::Texture2D gliderTex;
    ae3d::Material material;
    ae3d::Mesh cubeMesh;
};
