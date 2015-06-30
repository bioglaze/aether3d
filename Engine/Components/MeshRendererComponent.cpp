#include "MeshRendererComponent.hpp"
#include <vector>
#include "Mesh.hpp"
#include "GfxDevice.hpp"
#include "VertexBuffer.hpp"
#include "SubMesh.hpp"
#include "Renderer.hpp"

#include "Texture2D.hpp"

extern ae3d::Renderer renderer;

std::vector< ae3d::MeshRendererComponent > meshRendererComponents;
unsigned nextFreeMeshRendererComponent = 0;

unsigned ae3d::MeshRendererComponent::New()
{
    if (nextFreeMeshRendererComponent == meshRendererComponents.size())
    {
        meshRendererComponents.resize( meshRendererComponents.size() + 10 );
    }
    
    return nextFreeMeshRendererComponent++;
}

ae3d::MeshRendererComponent* ae3d::MeshRendererComponent::Get( unsigned index )
{
    return &meshRendererComponents[index];
}

void ae3d::MeshRendererComponent::Render( const float* projectionViewMatrix )
{
    std::vector< SubMesh >& subMeshes = mesh->GetSubMeshes();

    for (auto& subMesh : subMeshes)
    {
        const ae3d::Texture2D* texture = ae3d::Texture2D::GetDefaultTexture();

        GfxDevice::SetBlendMode( ae3d::GfxDevice::BlendMode::Off );

        renderer.builtinShaders.spriteRendererShader.Use();
        renderer.builtinShaders.spriteRendererShader.SetMatrix( "_ProjectionModelMatrix", projectionViewMatrix );
        renderer.builtinShaders.spriteRendererShader.SetTexture("textureMap", texture, 0);

        subMesh.vertexBuffer.Bind();
        subMesh.vertexBuffer.Draw();
    }
}
