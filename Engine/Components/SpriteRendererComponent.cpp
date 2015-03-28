#include "SpriteRendererComponent.hpp"
#include <vector>
#include "Renderer.hpp"
#include "Texture2D.hpp"
#include "Vec3.hpp"
#include "VertexBuffer.hpp"

extern ae3d::Renderer renderer;

ae3d::SpriteRendererComponent spriteRendererComponents[100];
int nextFreeSpriteComponent = 0;

struct Sprite
{
    ae3d::Texture2D* texture = nullptr;
    ae3d::Vec3 position;
};

struct ae3d::SpriteRendererComponent::Impl
{
    void BuildRenderQueue();
    
    bool renderQueueIsDirty = true;
    std::vector< Sprite > sprites;
    VertexBuffer vertexBuffer;
};


int ae3d::SpriteRendererComponent::New()
{
    return nextFreeSpriteComponent++;
}

ae3d::SpriteRendererComponent* ae3d::SpriteRendererComponent::Get(int index)
{
    return &spriteRendererComponents[index];
}

void ae3d::SpriteRendererComponent::Clear()
{
    m().sprites.clear();
    m().renderQueueIsDirty = true;
}

void ae3d::SpriteRendererComponent::SetTexture( Texture2D* aTexture, const Vec3& position )
{
    Sprite sprite;
    sprite.texture = aTexture;
    sprite.position = position;
    m().sprites.emplace_back( sprite );
    m().renderQueueIsDirty = true;
}

void ae3d::SpriteRendererComponent::Impl::BuildRenderQueue()
{
    //vertexBuffer.Clear();
    std::vector< VertexBuffer::VertexPT > vertices( sprites.size() * 4 );
    std::vector< VertexBuffer::Face > faces( sprites.size() * 2 );
    
    for (std::size_t i = 0; i < sprites.size(); ++i)
    {
        const float width  = sprites[ i ].texture->GetWidth();
        const float height = sprites[ i ].texture->GetHeight();
        
        vertices[ i * 4 + 0 ].position = sprites[ i ].position;
        vertices[ i * 4 + 1 ].position = sprites[ i ].position + Vec3( width, 0, 0 );
        vertices[ i * 4 + 2 ].position = sprites[ i ].position + Vec3( width, height, 0 );
        vertices[ i * 4 + 3 ].position = sprites[ i ].position + Vec3( 0, height, 0 );

        vertices[ i * 4 + 0 ].u = 0;
        vertices[ i * 4 + 0 ].v = 0;

        vertices[ i * 4 + 1 ].u = 1;
        vertices[ i * 4 + 1 ].v = 0;

        vertices[ i * 4 + 2 ].u = 1;
        vertices[ i * 4 + 2 ].v = 1;

        vertices[ i * 4 + 3 ].u = 0;
        vertices[ i * 4 + 3 ].v = 1;

        faces[ i * 2 + 0 ].a = i * 4 + 0;
        faces[ i * 2 + 0 ].b = i * 4 + 1;
        faces[ i * 2 + 0 ].c = i * 4 + 2;

        faces[ i * 2 + 1 ].a = i * 4 + 2;
        faces[ i * 2 + 1 ].b = i * 4 + 3;
        faces[ i * 2 + 1 ].c = i * 4 + 0;
    }
    
    vertexBuffer.Generate( faces.data(), static_cast<int>(faces.size()), vertices.data(), static_cast<int>(vertices.size()) );
    renderQueueIsDirty = false;
}

void ae3d::SpriteRendererComponent::Render( const float* projectionMatrix )
{
    if (m().renderQueueIsDirty)
    {
        m().BuildRenderQueue();
    }
    
    renderer.builtinShaders.spriteRendererShader.Use();
    renderer.builtinShaders.spriteRendererShader.SetMatrix( "_ProjectionMatrix", projectionMatrix );
    m().vertexBuffer.Draw();
}
