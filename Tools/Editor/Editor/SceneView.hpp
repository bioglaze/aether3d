#pragma once
#include "GameObject.hpp"
#include "FileSystem.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "Shader.hpp"
#include "Material.hpp"
#include "Texture2D.hpp"

class SceneView
{
public:
    void Init( int width, int height );
    void Render();

private:
    ae3d::GameObject camera;
    ae3d::Scene scene;
    ae3d::Shader unlitShader;
    
    // TODO: Test content, remove when stuff works.
    ae3d::GameObject cube;
    ae3d::Texture2D gliderTex;
    ae3d::Material material;
    ae3d::Mesh cubeMesh;
};
