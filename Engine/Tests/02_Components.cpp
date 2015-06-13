#include <iostream>
#include "CameraComponent.hpp"
#include "GameObject.hpp"
#include "RenderTexture.hpp"
#include "SpriteRendererComponent.hpp"
#include "System.hpp"
#include "TransformComponent.hpp"
#include "TextRendererComponent.hpp"
#include "Vec3.hpp"

using namespace ae3d;

GameObject gGo;
RenderTexture2D gRenderTexture;

void TestCamera()
{
    gGo.AddComponent< CameraComponent >();
    gGo.GetComponent< CameraComponent >()->SetTargetTexture( &gRenderTexture );
    System::Assert( gGo.GetComponent< CameraComponent >()->GetTargetTexture() == &gRenderTexture, "camera render texture failed" );
}

void TestTransform()
{
    const Vec3 pos{ 1, 2, 3 };
    gGo.AddComponent< TransformComponent >();
    gGo.GetComponent< TransformComponent >()->SetLocalPosition( pos );
    System::Assert( gGo.GetComponent< TransformComponent >()->GetLocalPosition().IsAlmost( pos ), "transform position failed" );
}

void TestText()
{
    gGo.AddComponent< TextRendererComponent >();
    gGo.GetComponent< TextRendererComponent >()->SetText( "aether3d" );
    gGo.GetComponent< TextRendererComponent >()->SetText( "a" );
    gGo.GetComponent< TextRendererComponent >()->SetText( "" );
    gGo.GetComponent< TextRendererComponent >()->SetText( nullptr );
}

void TestSprite()
{
    gGo.AddComponent< SpriteRendererComponent >();
    gGo.GetComponent< SpriteRendererComponent >()->SetTexture( nullptr, { 0, 0, 0 }, { 10, 10, 1 }, { 1, 0, 0, 1 } );
}

int main()
{
    TestCamera();
    TestTransform();
    TestText();
    TestSprite();
}

    
