#include "TextRendererComponent.hpp"
#include <vector>
#include <sstream>
#include "Font.hpp"
#include "GfxDevice.hpp"
#include "Renderer.hpp"
#include "Shader.hpp"
#include "VertexBuffer.hpp"
#include "Vec3.hpp"

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
    Shader* shader = &renderer.builtinShaders.spriteRendererShader;
};

ae3d::TextRendererComponent::TextRendererComponent()
{
    new(&_storage)Impl();
}

ae3d::TextRendererComponent::TextRendererComponent( const TextRendererComponent& other )
{
    new(&_storage)Impl();
    reinterpret_cast<Impl&>(_storage) = reinterpret_cast<Impl const&>(other._storage);
    gameObject = other.gameObject;
}

ae3d::TextRendererComponent& ae3d::TextRendererComponent::operator=( const TextRendererComponent& other )
{
    new(&_storage)Impl();
    reinterpret_cast<Impl&>(_storage) = reinterpret_cast<Impl const&>(other._storage);
    gameObject = other.gameObject;
    return *this;
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
    m().text = aText == nullptr ? std::string( "" ) : std::string( aText );
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
        if (!m().text.empty())
        {
            m().font->CreateVertexBuffer( m().text.c_str(), m().color, m().vertexBuffer );
        }

        m().isDirty = false;
    }

    if (m().vertexBuffer.IsGenerated() && !m().text.empty())
    {
        auto shader = m().shader;
        shader->Use();
        shader->SetMatrix( "_ProjectionModelMatrix", projectionModelMatrix );
        shader->SetTexture( "textureMap", m().font->GetTexture(), 0 );

        GfxDevice::Draw( m().vertexBuffer, 0, m().vertexBuffer.GetFaceCount() / 3, *m().shader, ae3d::GfxDevice::BlendMode::AlphaBlend,
                         ae3d::GfxDevice::DepthFunc::LessOrEqualWriteOff, ae3d::GfxDevice::CullMode::Off );
    }
}

void ae3d::TextRendererComponent::SetShader( ShaderType aShaderType )
{
    m().shader = aShaderType == ShaderType::Sprite ? &renderer.builtinShaders.spriteRendererShader : &renderer.builtinShaders.sdfShader;
}

std::string ae3d::TextRendererComponent::GetSerialized() const
{
    std::stringstream outStream;
    outStream << "textrenderer\n";
    const auto& color = m().color;
    outStream << "color " << color.x << " " << color.y << " " << color.z << " " << color.w << "\n";
    outStream << "text " << m().text << "\n\n";
    return outStream.str();
}
