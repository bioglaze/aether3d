// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "MeshRendererComponent.hpp"
#include <string>
#include <vector>
#include "Frustum.hpp"
#include "GfxDevice.hpp"
#include "Matrix.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "Shader.hpp"
#include "System.hpp"
#include "SubMesh.hpp"
#include "VertexBuffer.hpp"
#include "Vec3.hpp"

using namespace ae3d;

namespace GfxDeviceGlobal
{
    extern PerObjectUboStruct perObjectUboStruct;
}

namespace MathUtil
{
    void GetMinMax( const Vec3* aPoints, int count, Vec3& outMin, Vec3& outMax );
    void GetCorners( const Vec3& min, const Vec3& max, Vec3 outCorners[ 8 ] );
}

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

Material* ae3d::MeshRendererComponent::GetMaterial( int subMeshIndex )
{
    return subMeshIndex < (int)materials.count ? materials[ subMeshIndex ] : nullptr;
}

ae3d::MeshRendererComponent* ae3d::MeshRendererComponent::Get( unsigned index )
{
    return &meshRendererComponents[ index ];
}

std::string GetSerialized( ae3d::MeshRendererComponent* component )
{
    std::string outStr( "meshrenderer\nmeshpath " );
    
    if (component->GetMesh())
    {
        outStr += std::string( component->GetMesh()->GetPath() );
    }
    else
    {
        outStr += "none";
    }

    outStr += "\nmeshrenderer_cast_shadow ";
    outStr += component->CastsShadow() ? "1" : "0";
    outStr += "\nmeshrenderer_enabled ";
    outStr += component->IsEnabled() ? "1" : "0";
    outStr += "\n\n";
    
    return outStr;
}

void ae3d::MeshRendererComponent::Cull( const class Frustum& cameraFrustum, const struct Matrix44& localToWorld )
{
    if (!mesh)
    {
        return;
    }

    isCulled = false;
    
    Vec3 aabbWorld[ 8 ];
    MathUtil::GetCorners( mesh->GetAABBMin(), mesh->GetAABBMax(), aabbWorld );
    
    for (std::size_t v = 0; v < 8; ++v)
    {
        Matrix44::TransformPoint( aabbWorld[ v ], localToWorld, &aabbWorld[ v ] );
    }
    
    Vec3 aabbMinWorld;
    Vec3 aabbMaxWorld;
    MathUtil::GetMinMax( aabbWorld, 8, aabbMinWorld, aabbMaxWorld );
    
    if (!cameraFrustum.BoxInFrustum( aabbMinWorld, aabbMaxWorld ))
    {
        isCulled = true;
        return;
    }

    int subMeshCount = 0;
    SubMesh* subMeshes = mesh->GetSubMeshes( subMeshCount);

    for (int subMeshIndex = 0; subMeshIndex < subMeshCount; ++subMeshIndex)
    {
        isSubMeshCulled[ subMeshIndex ] = false;

        if (materials[ subMeshIndex ] == nullptr || !materials[ subMeshIndex ]->IsValidShader())
        {
            isSubMeshCulled[ subMeshIndex ] = true;
            continue;
        }
        
        Vec3 meshAabbMinWorld = subMeshes[ subMeshIndex ].aabbMin;
        Vec3 meshAabbMaxWorld = subMeshes[ subMeshIndex ].aabbMax;
        
        Vec3 meshAabbWorld[ 8 ];
        MathUtil::GetCorners( meshAabbMinWorld, meshAabbMaxWorld, meshAabbWorld );
        
        for (std::size_t v = 0; v < 8; ++v)
        {
            Matrix44::TransformPoint( meshAabbWorld[ v ], localToWorld, &meshAabbWorld[ v ] );
        }
        
        MathUtil::GetMinMax( meshAabbWorld, 8, meshAabbMinWorld, meshAabbMaxWorld );
        
        if (!cameraFrustum.BoxInFrustum( meshAabbMinWorld, meshAabbMaxWorld ))
        {
            isSubMeshCulled[ subMeshIndex ] = true;
        }
    }
}

void ae3d::MeshRendererComponent::ApplySkin( unsigned subMeshIndex )
{
    int subMeshCount = 0;
    SubMesh* subMeshes = mesh->GetSubMeshes( subMeshCount );

    if (!subMeshes[ subMeshIndex ].joints.empty())
    {
        for (std::size_t j = 0; j < subMeshes[ subMeshIndex ].joints.size(); ++j)
        {
            const auto& joint = subMeshes[ subMeshIndex ].joints[ j ];
            
            if (!joint.animTransforms.empty())
            {
                const std::size_t frames = joint.animTransforms.size();
                Matrix44::Multiply( joint.globalBindposeInverse,
                                   joint.animTransforms[ animFrame % frames ],
                                   GfxDeviceGlobal::perObjectUboStruct.boneMatrices[ j ] );
            }
        }
    }

}

void ae3d::MeshRendererComponent::Render( const Matrix44& localToView, const Matrix44& localToClip, const Matrix44& localToWorld,
                                          const Matrix44& shadowView, const Matrix44& shadowProjection, Shader* overrideShader,
                                          Shader* overrideSkinShader, RenderType renderType )
{
    if (isCulled || !mesh || !isEnabled)
    {
        return;
    }
    
	int subMeshCount = 0;
    SubMesh* subMeshes = mesh->GetSubMeshes( subMeshCount );

    for (int subMeshIndex = 0; subMeshIndex < subMeshCount; ++subMeshIndex)
    {
        if (isSubMeshCulled[ subMeshIndex ])
        {
            continue;
        }
        
        if (materials[ subMeshIndex ]->GetBlendingMode() != Material::BlendingMode::Off && renderType == RenderType::Opaque)
        {
            continue;
        }

        if (materials[ subMeshIndex ]->GetBlendingMode() == Material::BlendingMode::Off && renderType == RenderType::Transparent)
        {
            continue;
        }

        Shader* shader = overrideShader ? overrideShader : materials[ subMeshIndex ]->GetShader();
        
        if (overrideSkinShader && !subMeshes[ subMeshIndex ].joints.empty())
        {
            shader = overrideSkinShader;
        }
        
        GfxDevice::CullMode cullMode = GfxDevice::CullMode::Back;
        GfxDevice::BlendMode blendMode = GfxDevice::BlendMode::Off;

#if AE3D_OPENVR
        GfxDeviceGlobal::perObjectUboStruct.isVR = 1;
#endif

        if (overrideShader)
        {
            shader->Use();
            GfxDeviceGlobal::perObjectUboStruct.localToClip = localToClip;
            GfxDeviceGlobal::perObjectUboStruct.localToView = localToView;
            ApplySkin( subMeshIndex );
        }
        else
        {
            Matrix44 localToShadowClip;
            
            Matrix44::Multiply( localToWorld, shadowView, localToShadowClip );
            Matrix44::Multiply( localToShadowClip, shadowProjection, localToShadowClip );
#ifndef RENDERER_METAL
            Matrix44::Multiply( localToShadowClip, Matrix44::bias, localToShadowClip );
#endif
            materials[ subMeshIndex ]->Apply();
            
            GfxDeviceGlobal::perObjectUboStruct.localToClip = localToClip;
            GfxDeviceGlobal::perObjectUboStruct.localToView = localToView;
            GfxDeviceGlobal::perObjectUboStruct.localToWorld = localToWorld;
            GfxDeviceGlobal::perObjectUboStruct.localToShadowClip = localToShadowClip;

            ApplySkin( subMeshIndex );
            
            if (!materials[ subMeshIndex ]->IsBackFaceCulled())
            {
                cullMode = GfxDevice::CullMode::Off;
            }
            
            if (materials[ subMeshIndex ]->GetBlendingMode() == Material::BlendingMode::Alpha)
            {
                blendMode = GfxDevice::BlendMode::AlphaBlend;
            }
        }
        
        GfxDevice::DepthFunc depthFunc;
        
        if (materials[ subMeshIndex ]->GetDepthFunction() == Material::DepthFunction::LessOrEqualWriteOn)
        {
            depthFunc = GfxDevice::DepthFunc::LessOrEqualWriteOn;
        }
        else if (materials[ subMeshIndex ]->GetDepthFunction() == Material::DepthFunction::NoneWriteOff)
        {
            depthFunc = GfxDevice::DepthFunc::NoneWriteOff;
        }
        else
        {
            System::Assert( false, "material has unhandled depth function" );
            depthFunc = GfxDevice::DepthFunc::NoneWriteOff;
        }
        
        GfxDevice::Draw( subMeshes[ subMeshIndex ].vertexBuffer, 0, subMeshes[ subMeshIndex ].vertexBuffer.GetFaceCount() / 3,
                         *shader, blendMode, depthFunc, cullMode, isWireframe ? GfxDevice::FillMode::Wireframe : GfxDevice::FillMode::Solid, GfxDevice::PrimitiveTopology::Triangles );

        if (isAabbDrawingEnabled)
        {
            Vec3 aabb[ 8 ];
            MathUtil::GetCorners( mesh->GetAABBMin(), mesh->GetAABBMax(), aabb );
    
            Vec3 aabbMin, aabbMax;
            MathUtil::GetMinMax( aabb, 8, aabbMin, aabbMax );

            const int lineCount = 24;
            Vec3 lines[ lineCount ] =
            {
                Vec3( aabbMin.x, aabbMin.y, aabbMin.z ) * 1.1f,
                Vec3( aabbMax.x, aabbMin.y, aabbMin.z ) * 1.1f,
                Vec3( aabbMax.x, aabbMin.y, aabbMax.z ) * 1.1f,
                Vec3( aabbMin.x, aabbMin.y, aabbMax.z ) * 1.1f,

                Vec3( aabbMin.x, aabbMax.y, aabbMin.z ) * 1.1f,
                Vec3( aabbMax.x, aabbMax.y, aabbMin.z ) * 1.1f,
                Vec3( aabbMax.x, aabbMax.y, aabbMax.z ) * 1.1f,
                Vec3( aabbMin.x, aabbMax.y, aabbMax.z ) * 1.1f,

                Vec3( aabbMin.x, aabbMax.y, aabbMin.z ) * 1.1f,
                Vec3( aabbMin.x, aabbMax.y, aabbMax.z ) * 1.1f,
                Vec3( aabbMax.x, aabbMax.y, aabbMin.z ) * 1.1f,
                Vec3( aabbMax.x, aabbMax.y, aabbMax.z ) * 1.1f,

                Vec3( aabbMin.x, aabbMin.y, aabbMin.z ) * 1.1f,
                Vec3( aabbMin.x, aabbMin.y, aabbMax.z ) * 1.1f,
                Vec3( aabbMax.x, aabbMin.y, aabbMin.z ) * 1.1f,
                Vec3( aabbMax.x, aabbMin.y, aabbMax.z ) * 1.1f,

                Vec3( aabbMin.x, aabbMin.y, aabbMin.z ) * 1.1f,
                Vec3( aabbMin.x, aabbMax.y, aabbMin.z ) * 1.1f,
                Vec3( aabbMax.x, aabbMin.y, aabbMin.z ) * 1.1f,
                Vec3( aabbMax.x, aabbMax.y, aabbMin.z ) * 1.1f,

                Vec3( aabbMin.x, aabbMin.y, aabbMax.z ) * 1.1f,
                Vec3( aabbMin.x, aabbMax.y, aabbMax.z ) * 1.1f,
                Vec3( aabbMax.x, aabbMin.y, aabbMax.z ) * 1.1f,
                Vec3( aabbMax.x, aabbMax.y, aabbMax.z ) * 1.1f,
            };

            GfxDevice::UpdateLineBuffer( aabbLineHandle, lines, lineCount, Vec3( 1, 0, 0 ) );
            GfxDevice::DrawLines( aabbLineHandle, *shader );
        }
    }
}

void ae3d::MeshRendererComponent::SetMaterial( Material* material, unsigned subMeshIndex )
{
    if (subMeshIndex < materials.count )
    {
        materials[ subMeshIndex ] = material;
    }
}

void ae3d::MeshRendererComponent::EnableBoundingBoxDrawing( bool enable )
{
    isAabbDrawingEnabled = enable;

    if (enable && aabbLineHandle == -1)
    {
        constexpr unsigned LineCount = 24;
        Vec3 lines[ LineCount ];
    
        for (unsigned i = 0; i < LineCount / 2; ++i)
        {
            lines[ i * 2 + 0 ] = Vec3( 0, 0, 0 );
            lines[ i * 2 + 1 ] = Vec3( 0, 0, 0 );
        }

        aabbLineHandle = GfxDevice::CreateLineBuffer( lines, LineCount, Vec3( 1, 1, 1 ) );
    }
}

bool ae3d::MeshRendererComponent::IsBoundingBoxDrawingEnabled() const
{
    return isAabbDrawingEnabled;
}

void ae3d::MeshRendererComponent::SetMesh( Mesh* aMesh )
{
    mesh = aMesh;

    if (mesh != nullptr)
    {
        int subMeshCount = 0;
        mesh->GetSubMeshes( subMeshCount );
        materials.Allocate( subMeshCount );
        isSubMeshCulled.Allocate( subMeshCount );
    }
}
