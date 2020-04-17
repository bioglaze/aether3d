// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "Material.hpp"
#include "GfxDevice.hpp"
#include "Texture2D.hpp"
#include "RenderTexture.hpp"
#include "Shader.hpp"

ae3d::RenderTexture* ae3d::Material::sTexRT;

namespace GfxDeviceGlobal
{
    extern PerObjectUboStruct perObjectUboStruct;
}

bool ae3d::Material::IsValidShader() const
{
    return shader && shader->IsValid();
}

void ae3d::Material::Apply()
{
    if (shader == nullptr)
    {
        return;
    }

    shader->Use();
    
    for (int slot = 0; slot < TEXTURE_SLOT_COUNT; ++slot)
    {
        shader->SetTexture( tex2dSlots[ slot ], slot );
    }

    for (int slot = 0; slot < TEXTURE_SLOT_COUNT; ++slot)
    {
        if (texCubeSlots[ slot ])
        {
            shader->SetTexture( texCubeSlots[ slot ], slot );
        }

        if (rtSlots[ slot ])
        {
            shader->SetRenderTexture( rtSlots[ slot ], slot );
        }
    }

    if (sTexRT)
    {
        int texUnit = 1;

#if RENDERER_METAL
        if (sTexRT->IsCube())
        {
            shader->SetRenderTexture( sTexRT, 2 );
        }
        else
        {
            shader->SetRenderTexture( sTexRT, texUnit );
        }
#else
        shader->SetRenderTexture( sTexRT, texUnit );
#endif
        ++texUnit;
    }

    if (depthUnits > 0.0001f || depthUnits < -0.0001f || depthFactor > 0.0001f || depthFactor < -0.0001f)
    {
        GfxDevice::SetPolygonOffset( true, depthFactor, depthUnits );
    }
    else
    {
        GfxDevice::SetPolygonOffset( false, 0, 0 );
    }

    GfxDeviceGlobal::perObjectUboStruct.f0 = f0;
}

void ae3d::Material::SetShader( Shader* aShader )
{
    shader = aShader;
}

void ae3d::Material::SetF0( float af0 )
{
    f0 = af0;
}

void ae3d::Material::SetTexture( Texture2D* texture, int slot )
{
    if (0 <= slot && slot < TEXTURE_SLOT_COUNT)
    {
        tex2dSlots[ slot ] = texture;
    }
}

void ae3d::Material::SetTexture( TextureCube* texture )
{
    texCubeSlots[ 12 ] = texture;
}

void ae3d::Material::SetRenderTexture( RenderTexture* renderTexture, int slot )
{
    rtSlots[ slot ] = renderTexture;
}

void ae3d::Material::SetGlobalRenderTexture( RenderTexture* renderTexture )
{
    sTexRT = renderTexture;
}
