#include "TextRendererComponent.hpp"
#include <vector>
#include "Font.hpp"
#include "Renderer.hpp"
#include "VertexBuffer.hpp"

extern ae3d::Renderer renderer;

std::vector< ae3d::TextRendererComponent > textComponents;
unsigned nextFreeTextComponent = 0;

int ae3d::TextRendererComponent::New()
{
    if (nextFreeTextComponent == textComponents.size())
    {
        textComponents.resize( textComponents.size() + 10 );
    }
    
    return nextFreeTextComponent++;
}

ae3d::TextRendererComponent* ae3d::TextRendererComponent::Get( unsigned index )
{
    return &textComponents[ index ];
}

struct ae3d::TextRendererComponent::Impl
{
    VertexBuffer vertexBuffer;
    std::string text;
    Font* font = nullptr;
};

ae3d::TextRendererComponent::TextRendererComponent()
{
    new(&_storage)Impl();
}

ae3d::TextRendererComponent::~TextRendererComponent()
{
    reinterpret_cast< Impl* >(&_storage)->~Impl();
}

void ae3d::TextRendererComponent::SetText( const char* aText )
{
    m().text = aText;

    if (m().font)
    {
        // TODO: Dirty flag.
        m().font->CreateVertexBuffer( aText, m().vertexBuffer );
    }
}

void ae3d::TextRendererComponent::SetFont( Font* aFont )
{
    m().font = aFont;

    if (m().font)
    {
        // TODO: Dirty flag.
        m().font->CreateVertexBuffer( m().text.c_str()  , m().vertexBuffer );
    }
}

void ae3d::TextRendererComponent::Render( const float* projectionModelMatrix )
{
    renderer.builtinShaders.spriteRendererShader.Use();
    renderer.builtinShaders.spriteRendererShader.SetMatrix( "_ProjectionModelMatrix", projectionModelMatrix );
    renderer.builtinShaders.spriteRendererShader.SetTexture("textureMap", m().font->GetTexture(), 0);

    m().vertexBuffer.Bind();
    m().vertexBuffer.Draw();
}
