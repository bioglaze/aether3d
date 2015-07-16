#include "Material.hpp"
#include "Texture2D.hpp"
#include "Shader.hpp"

void ae3d::Material::Apply()
{
    if (shader == nullptr)
    {
        return;
    }

    shader->Use();
    
    for (const auto& vec : vec4s)
    {
        shader->SetVector4( vec.first.c_str(), &vec.second.x );
    }

    int texUnit = 0;
    
    for (const auto& tex2d : tex2ds)
    {
        shader->SetTexture( tex2d.first.c_str(), tex2d.second, texUnit );
        ++texUnit;
    }

    for (const auto& mat4 : mat4s)
    {
        shader->SetMatrix( mat4.first.c_str(), mat4.second.m );
    }
}

void ae3d::Material::SetMatrix( const char* name, const Matrix44& matrix )
{
    mat4s[ name ] = matrix;
}

void ae3d::Material::SetShader( Shader* aShader )
{
    shader = aShader;
}

void ae3d::Material::SetTexture( const char* name, Texture2D* texture )
{
    tex2ds[ name ] = texture;
}

void ae3d::Material::SetVector( const char* name, const Vec4& vec )
{
    vec4s[ name ] = vec;
}

