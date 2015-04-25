#include "TextRendererComponent.hpp"
#include <vector>
#include "Font.hpp"
#include "GfxDevice.hpp"
#include "Renderer.hpp"
#include "VertexBuffer.hpp"
#include "Vec3.hpp"
#include "System.hpp"
extern ae3d::Renderer renderer;

std::vector< ae3d::TextRendererComponent > textComponents;
unsigned nextFreeTextComponent = 0;

unsigned ae3d::TextRendererComponent::New()
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
    std::string text = "text";
    Font* font = nullptr;
    Vec4 color = { 1, 1, 1, 1 };
    bool isDirty = true;
};

ae3d::TextRendererComponent::TextRendererComponent()
{
    new(&_storage)Impl();
}

ae3d::TextRendererComponent::~TextRendererComponent()
{
    reinterpret_cast< Impl* >(&_storage)->~Impl();
}

void ae3d::TextRendererComponent::SetColor( const Vec4& aColor )
{
    m().color = aColor;
    m().isDirty = true;
}

void ae3d::TextRendererComponent::SetText( const char* aText )
{
    m().text = aText == nullptr ? "" : aText;
    m().isDirty = true;
}

void ae3d::TextRendererComponent::SetFont( Font* aFont )
{
    m().font = aFont;
    m().isDirty = true;
}

void ae3d::TextRendererComponent::Render( const float* projectionModelMatrix )
{
    if (m().font == nullptr)
    {
        return;
    }

    if (m().isDirty)
    {
        m().font->CreateVertexBuffer( m().text.c_str(), m().color, m().vertexBuffer );
        m().isDirty = false;
    }

    renderer.builtinShaders.spriteRendererShader.Use();
    renderer.builtinShaders.spriteRendererShader.SetMatrix( "_ProjectionModelMatrix", projectionModelMatrix );
    renderer.builtinShaders.spriteRendererShader.SetTexture( "textureMap", m().font->GetTexture(), 0 );

    GfxDevice::SetBlendMode( ae3d::GfxDevice::BlendMode::AlphaBlend );
    
    m().vertexBuffer.Bind();
    m().vertexBuffer.Draw();
}
