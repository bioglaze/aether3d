#include "SpriteRendererComponent.hpp"
#include <algorithm>
#include <sstream>
#include <vector>
#include "GfxDevice.hpp"
#include "Renderer.hpp"
#include "RenderTexture.hpp"
#include "Texture2D.hpp"
#include "TextureBase.hpp"
#include "Vec3.hpp"
#include "VertexBuffer.hpp"
#include "System.hpp"

extern ae3d::Renderer renderer;

std::vector<ae3d::SpriteRendererComponent> spriteRendererComponents;
unsigned nextFreeSpriteComponent = 0;

namespace GfxDeviceGlobal
{
    extern PerObjectUboStruct perObjectUboStruct;
}

struct Drawable
{
    ae3d::TextureBase* texture = ae3d::Texture2D::GetDefaultTexture();
    int bufferStart = 0;
    int bufferEnd = 0;
};

struct Sprite
{
    ae3d::TextureBase* texture = ae3d::Texture2D::GetDefaultTexture();
    ae3d::Vec3 position;
    ae3d::Vec3 dimension;
    ae3d::Vec4 tint;
};

namespace
{
void CreateVertexBuffer( std::vector< Sprite >& sprites, ae3d::VertexBuffer& outVertexBuffer )
{
    const int quadVertexCount = 4;
    const int quadFaceCount = 2;
    std::vector< ae3d::VertexBuffer::VertexPTC > vertices( sprites.size() * quadVertexCount );
    std::vector< ae3d::VertexBuffer::Face > faces( sprites.size() * quadFaceCount );
    
    // TODO: evaluate perf and consider other containers.
    std::sort( sprites.begin(), sprites.end(), [](const Sprite& a, const Sprite& b) { return a.texture->GetID() < b.texture->GetID(); } );
    
    for (unsigned short i = 0; i < static_cast<unsigned short>(sprites.size()); ++i)
    {
        const auto& dim = sprites[ i ].dimension;
        vertices[ i * quadVertexCount + 0 ].position = sprites[ i ].position;
        vertices[ i * quadVertexCount + 1 ].position = sprites[ i ].position + ae3d::Vec3( dim.x, 0, 0 );
        vertices[ i * quadVertexCount + 2 ].position = sprites[ i ].position + ae3d::Vec3( dim.x, dim.y, 0 );
        vertices[ i * quadVertexCount + 3 ].position = sprites[ i ].position + ae3d::Vec3( 0, dim.y, 0 );
        
        for (int v = 0; v < quadVertexCount; ++v)
        {
            vertices[ i * quadVertexCount + v ].color = sprites[i].tint;
        }
        
        const ae3d::Vec4& so = sprites[i].texture->GetScaleOffset();

        const float u0 = 0.0f * so.x + so.z;
        const float u1 = 1.0f * so.x + so.z;

        const float v0 = 0.0f * so.y + so.w;
        const float v1 = 1.0f * so.y + so.w;

        vertices[ i * quadVertexCount + 0 ].u = u0;
        vertices[ i * quadVertexCount + 0 ].v = v0;
        
        vertices[ i * quadVertexCount + 1 ].u = u1;
        vertices[ i * quadVertexCount + 1 ].v = v0;
        
        vertices[ i * quadVertexCount + 2 ].u = u1;
        vertices[ i * quadVertexCount + 2 ].v = v1;
        
        vertices[ i * quadVertexCount + 3 ].u = u0;
        vertices[ i * quadVertexCount + 3 ].v = v1;
        
        auto& tri1 = faces[ i * quadFaceCount + 0 ];
        tri1.a = i * quadVertexCount + 0;
        tri1.b = i * quadVertexCount + 1;
        tri1.c = i * quadVertexCount + 2;
        
        auto& tri2 = faces[ i * quadFaceCount + 1 ];
        tri2.a = i * quadVertexCount + 2;
        tri2.b = i * quadVertexCount + 3;
        tri2.c = i * quadVertexCount + 0;
    }
    
    outVertexBuffer.Generate( faces.data(), static_cast<int>(faces.size()), vertices.data(), static_cast<int>(vertices.size()) );
    outVertexBuffer.SetDebugName( "sprite buffer" );
}

void CreateDrawables( const std::vector< Sprite >& sprites, std::vector< Drawable >& outDrawables )
{
    outDrawables.clear();
    
    if (sprites.empty())
    {
        return;
    }
    
    outDrawables.emplace_back( Drawable() );
    auto& back = outDrawables.back();
    back.texture = sprites[0].texture;
    back.bufferStart = 0;
    back.bufferEnd = 2;
    
    for (std::size_t s = 1; s < sprites.size(); ++s)
    {
        if (sprites[s].texture == sprites[s - 1].texture)
        {
            outDrawables.back().bufferEnd += 2;
        }
        else
        {
            const int oldEnd = outDrawables.back().bufferEnd;
            
            outDrawables.emplace_back( Drawable() );
            auto& back2 = outDrawables.back();
            back2.texture = sprites[s].texture;
            back2.bufferStart = oldEnd;
            back2.bufferEnd = oldEnd + 2;
        }
    }
}
}

struct RenderQueue
{
    void Clear();
    void Build();
    void Render( ae3d::GfxDevice::BlendMode blendMode, const float* localToClip );
    
    bool isDirty = true;
    std::vector< Sprite > sprites;
    std::vector< Drawable > drawables;
    ae3d::VertexBuffer vertexBuffer;
};

void RenderQueue::Clear()
{
    sprites.clear();
    isDirty = true;
}

void RenderQueue::Build()
{
    if (!sprites.empty())
    {
        CreateVertexBuffer( sprites, vertexBuffer );
        CreateDrawables( sprites, drawables );
    }

    isDirty = false;
}

void RenderQueue::Render( ae3d::GfxDevice::BlendMode blendMode, const float* localToClip )
{
    if (isDirty)
    {
        Build();
    }
    
    for (auto& drawable : drawables)
    {
        renderer.builtinShaders.spriteRendererShader.Use();
#if RENDERER_OPENGL
        GfxDeviceGlobal::perObjectUboStruct.localToClip.InitFrom( localToClip );
#else
        renderer.builtinShaders.spriteRendererShader.SetMatrix( "_LocalToClip", localToClip );
#endif
        
        if (drawable.texture->IsRenderTexture())
        {
            renderer.builtinShaders.spriteRendererShader.SetRenderTexture("textureMap", static_cast< ae3d::RenderTexture* >(drawable.texture), 0);
        }
        else
        {
            renderer.builtinShaders.spriteRendererShader.SetTexture("textureMap", static_cast< ae3d::Texture2D* >(drawable.texture), 0);
        }
        
        ae3d::GfxDevice::Draw( vertexBuffer, drawable.bufferStart, drawable.bufferEnd, renderer.builtinShaders.spriteRendererShader, blendMode,
                               ae3d::GfxDevice::DepthFunc::NoneWriteOff, ae3d::GfxDevice::CullMode::Off, ae3d::GfxDevice::FillMode::Solid, ae3d::GfxDevice::PrimitiveTopology::Triangles );
    }
}

struct ae3d::SpriteRendererComponent::Impl
{
    Impl() : opaqueRenderQueue(), transparentRenderQueue()
    {
        static_assert( sizeof( ae3d::SpriteRendererComponent::Impl ) <= ae3d::SpriteRendererComponent::StorageSize, "Impl too big!");
        static_assert( ae3d::SpriteRendererComponent::StorageAlign % alignof( ae3d::SpriteRendererComponent::Impl ) == 0, "Impl misaligned!");
    }

    RenderQueue opaqueRenderQueue;
    RenderQueue transparentRenderQueue;
    std::vector< SpriteInfo > spriteInfos;
};

unsigned ae3d::SpriteRendererComponent::New()
{
    if (nextFreeSpriteComponent == spriteRendererComponents.size())
    {
        spriteRendererComponents.resize( spriteRendererComponents.size() + 10 );
    }

    return nextFreeSpriteComponent++;
}

ae3d::SpriteInfo ae3d::SpriteRendererComponent::GetSpriteInfo( int index ) const
{
    if (index > 0 && index < static_cast< int >( m().spriteInfos.size() ))
    {
        return m().spriteInfos[ index ];
    }

    return SpriteInfo{ "", 0, 0, 0, 0, false };
}

ae3d::SpriteRendererComponent* ae3d::SpriteRendererComponent::Get( unsigned index )
{
    if (index >= spriteRendererComponents.size())
    {
        System::Print( "SpriteRendererComponent: invalid index: %u\n", index );
    }
    
    System::Assert( index < spriteRendererComponents.size(), "Tried to get a too high index for spriteRendererComponents." );
    return &spriteRendererComponents[ index ];
}

ae3d::SpriteRendererComponent::SpriteRendererComponent()
{
    new(&_storage)Impl();
}

ae3d::SpriteRendererComponent::~SpriteRendererComponent()
{
    reinterpret_cast< Impl* >(&_storage)->~Impl();
}

ae3d::SpriteRendererComponent::SpriteRendererComponent( const SpriteRendererComponent& other )
{
    new(&_storage)Impl();
    reinterpret_cast<Impl&>(_storage) = reinterpret_cast<Impl const&>(other._storage);
    gameObject = other.gameObject;
}

ae3d::SpriteRendererComponent& ae3d::SpriteRendererComponent::operator=( const SpriteRendererComponent& other )
{
    if (this == &other)
    {
        return *this;
    }

    new(&_storage)Impl();
    reinterpret_cast<Impl&>(_storage) = reinterpret_cast<Impl const&>(other._storage);
    gameObject = other.gameObject;
    return *this;
}

std::string ae3d::SpriteRendererComponent::GetSerialized() const
{
    std::stringstream outStream;
    outStream << "spriterenderer\n";
    outStream << "enabled" << isEnabled << "\n";
    
    for (std::size_t spriteIndex = 0; spriteIndex < m().spriteInfos.size(); ++spriteIndex)
    {
        const auto& info = m().spriteInfos[ spriteIndex ];
        outStream << "sprite " << info.path << " " << info.x << " " << info.y << " " << info.width << " " << info.height << "\n";
    }

    return outStream.str();
}

void ae3d::SpriteRendererComponent::Clear()
{
    m().opaqueRenderQueue.Clear();
    m().transparentRenderQueue.Clear();
}

void ae3d::SpriteRendererComponent::SetTexture( TextureBase* aTexture, const Vec3& position, const Vec3& dimensionPixels,
                                                 const Vec4& tintColor )
{
    if (aTexture == nullptr)
    {
        aTexture = Texture2D::GetDefaultTexture();
    }
    
    Sprite sprite;
    sprite.texture = aTexture;
    sprite.position = position;
    sprite.dimension = dimensionPixels;
    sprite.tint = tintColor;

    m().spriteInfos.emplace_back( SpriteInfo{ aTexture->GetPath(), position.x, position.y, dimensionPixels.x, dimensionPixels.y, true } );

    if (!aTexture->IsOpaque() || static_cast<int>(tintColor.w) != 1)
    {
        m().transparentRenderQueue.sprites.emplace_back( sprite );
        m().transparentRenderQueue.isDirty = true;
    }
    else
    {
        m().opaqueRenderQueue.sprites.emplace_back( sprite );
        m().opaqueRenderQueue.isDirty = true;
    }
}

void ae3d::SpriteRendererComponent::Render( const float* localToClip )
{
    if (!isEnabled || (m().transparentRenderQueue.sprites.empty() && m().opaqueRenderQueue.sprites.empty()))
    {
        return;
    }
        
    m().opaqueRenderQueue.Render( ae3d::GfxDevice::BlendMode::Off, localToClip );
    m().transparentRenderQueue.Render( ae3d::GfxDevice::BlendMode::AlphaBlend, localToClip );
}
