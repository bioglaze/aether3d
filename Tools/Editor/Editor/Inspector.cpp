// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "Inspector.hpp"
#include "AudioSourceComponent.hpp"
#include "AudioClip.hpp"
#include "CameraComponent.hpp"
#include "DecalRendererComponent.hpp"
#include "DirectionalLightComponent.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "Mesh.hpp"
#include "MeshRendererComponent.hpp"
#include "ParticleSystemComponent.hpp"
#include "PointLightComponent.hpp"
#include "SpotLightComponent.hpp"
#include "System.hpp"
#include "Texture2D.hpp"
#include "TransformComponent.hpp"
#include "TextRendererComponent.hpp"
#include <string.h>
#include <stdio.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#if RENDERER_METAL
#include "nuklear.h"
#else
#include "../../Samples/NuklearTest/nuklear.h"
#endif

using namespace ae3d;

void GetOpenPath( char* path, const char* extension );

struct VertexPTC
{
    float position[ 3 ];
    float uv[ 2 ];
    float col[ 4 ];
};

nk_draw_null_texture nullTexture;
Texture2D uiTextures[ 10 ]; // index 0 is font.
nk_context ctx;
nk_font_atlas atlas;
nk_buffer uiCommands;
char gameObjectNameField[ 64 ];
int gameObjectNameFieldLength;

static void DrawNuklear( int width, int height )
{
    nk_convert_config config = {};
    static const nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF( VertexPTC, position )},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF( VertexPTC, uv )},
        {NK_VERTEX_COLOR, NK_FORMAT_R32G32B32A32_FLOAT, NK_OFFSETOF( VertexPTC, col )},
        {NK_VERTEX_LAYOUT_END}
    };

    config.vertex_layout = vertex_layout;
    config.vertex_size = sizeof( VertexPTC );
    config.vertex_alignment = NK_ALIGNOF( VertexPTC );
    config.null = nullTexture;
    config.circle_segment_count = 22;
    config.curve_segment_count = 22;
    config.arc_segment_count = 22;
    config.global_alpha = 1.0f;
    config.shape_AA = NK_ANTI_ALIASING_OFF;
    config.line_AA = NK_ANTI_ALIASING_OFF;

    const int MAX_VERTEX_MEMORY = 512 * 1024;
    const int MAX_ELEMENT_MEMORY = 128 * 1024;

    void* vertices;
    void* elements;
    System::MapUIVertexBuffer( MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY, &vertices, &elements );
    memset( vertices, 0, MAX_VERTEX_MEMORY * sizeof( VertexPTC ) );
    memset( elements, 0, MAX_ELEMENT_MEMORY * 3 * 2 );
    
    nk_buffer vbuf, ebuf;
    nk_buffer_init_fixed( &vbuf, vertices, MAX_VERTEX_MEMORY * sizeof( VertexPTC ) );
    nk_buffer_init_fixed( &ebuf, elements, MAX_ELEMENT_MEMORY * 3 * 2 );

    const auto res = nk_convert( &ctx, &uiCommands, &vbuf, &ebuf, &config );
    System::Assert( res == NK_CONVERT_SUCCESS, "buffer conversion failed!" );

#if RENDERER_VULKAN
    for (int i = 0; i < MAX_VERTEX_MEMORY / (int)sizeof( VertexPTC ); ++i)
    {
        VertexPTC* vert = &(static_cast<VertexPTC*>(vertices))[ i ];
        vert->position[ 1 ] = height - vert->position[ 1 ];
    }
#endif

    System::UnmapUIVertexBuffer();
    
    const nk_draw_command* cmd = nullptr;
    int offset = 0;

    nk_draw_foreach( cmd, &ctx, &uiCommands )
    {
        if (cmd->elem_count == 0)
        {
            continue;
        }

        System::Assert( cmd->texture.id < 10, "Too high texture index!" );
        System::DrawUI( (int)(cmd->clip_rect.x),
#if RENDERER_VULKAN
                        (int)(cmd->clip_rect.y - cmd->clip_rect.h) + 30, // FIXME: + 30 fixes a scissoring issue with textfield.
#else
                        (int)((height - (int)(cmd->clip_rect.y + cmd->clip_rect.h))),
#endif
                        (int)(cmd->clip_rect.w),
                        (int)(cmd->clip_rect.h),
#if RENDERER_VULKAN
                        cmd->elem_count, &uiTextures[ cmd->texture.id ], offset, width, height );
#else
                        cmd->elem_count, &uiTextures[ cmd->texture.id ], offset, width, height * 2 );
#endif

        offset += cmd->elem_count / 3;
    }

    nk_clear( &ctx );
}

void Inspector::Init()
{
    ssao = false;
    bloom = false;
    textEditActive = false;
    bloomThreshold = 0.5f;
    bloomIntensity = 1.0f;

    nk_font_atlas_init_default( &atlas );
    nk_font_atlas_begin( &atlas );

    int atlasWidth = 0;
    int atlasHeight = 0;
    nk_font* nkFont = nk_font_atlas_add_default( &atlas, 21.0f, nullptr );
    const void* image = nk_font_atlas_bake( &atlas, &atlasWidth, &atlasHeight, NK_FONT_ATLAS_RGBA32 );

    uiTextures[ 0 ].LoadFromData( image, atlasWidth, atlasHeight, "Nuklear font", DataType::UByte );

    nk_font_atlas_end( &atlas, nk_handle_id( uiTextures[ 0 ].GetID() ), &nullTexture );
    
    nk_init_default( &ctx, &nkFont->handle );
    nk_buffer_init_default( &uiCommands );
    //ctx.style.text.color = nk_rgb(255, 255, 255);
    //ctx.style.tab.text = nk_rgb(195,5,5); // Tree title

    //ctx.style.property.label_normal = nk_rgb(195,5,5);
    //ctx.style.property.normal = nk_style_item_color( nk_rgb(216,6,6) );
    
    //ctx.style.selectable.normal = nk_style_item_color(nk_rgb(2,2,2)); // Tree view items
    ctx.style.selectable.hover = nk_style_item_color( nk_rgb( 56, 56, 56) );
    //ctx.style.selectable.pressed = nk_style_item_color(nk_rgb(206,206,206));
}

void Inspector::BeginInput()
{
    nk_input_begin( &ctx );
}

void Inspector::EndInput()
{
    nk_input_end( &ctx );
}

void Inspector::HandleMouseScroll( int y )
{
    nk_input_scroll( &ctx, nk_vec2( 0, y ) );
}

void Inspector::HandleLeftMouseClick( int x, int y, int state )
{
    nk_input_button( &ctx, NK_BUTTON_LEFT, x, y, state );
}

void Inspector::HandleChar( char ch )
{
    nk_input_char( &ctx, ch );
}

void Inspector::HandleKey( int key, int state )
{
    nk_input_key( &ctx, (nk_keys)key, state );
}

void Inspector::HandleMouseMotion( int x, int y )
{
    nk_input_motion( &ctx, x, y );
}

bool Inspector::IsTextEditActive()
{
    // TODO: Try nk_item_is_any_active( &ctx )
    return textEditActive;
}

void Inspector::SetTextEditText( const char* str )
{
    strncpy( gameObjectNameField, str, 64 );
}

void Inspector::Render( unsigned width, unsigned height, GameObject* gameObject, Command& outCommand, GameObject** gameObjects, unsigned goCount, ae3d::Material* material, int& outSelectedGameObject )
{
    outCommand = Command::Empty;
    outSelectedGameObject = -1;
    
    if (nk_begin( &ctx, "Inspector", nk_rect( 0, 0, 450, 1100 ), NK_WINDOW_BORDER | NK_WINDOW_TITLE ))
    {
        textEditActive = ctx.current->edit.active;

        nk_layout_row_static( &ctx, 40, 430, 1 );

        // Gameobject is selected.

        if (gameObject != nullptr)
        {
            //nk_label( &ctx, gameObject->GetName(), NK_TEXT_LEFT );
            nk_edit_string( &ctx, NK_EDIT_FIELD, gameObjectNameField, &gameObjectNameFieldLength, 64, nk_filter_default );
            gameObject->SetName( gameObjectNameField );
        }

        TransformComponent* transform = gameObject ? gameObject->GetComponent< TransformComponent >() : nullptr;
        MeshRendererComponent* meshRenderer = gameObject ? gameObject->GetComponent< MeshRendererComponent >() : nullptr;
        AudioSourceComponent* audioSource = gameObject ? gameObject->GetComponent< AudioSourceComponent >() : nullptr;
        CameraComponent* camera = gameObject ? gameObject->GetComponent< CameraComponent >() : nullptr;
        PointLightComponent* pointLight = gameObject ? gameObject->GetComponent< PointLightComponent >() : nullptr;
        SpotLightComponent* spotLight = gameObject ? gameObject->GetComponent< SpotLightComponent >() : nullptr;
        DirectionalLightComponent* dirLight = gameObject ? gameObject->GetComponent< DirectionalLightComponent >() : nullptr;
        ParticleSystemComponent* particleSystem = gameObject ? gameObject->GetComponent< ParticleSystemComponent >() : nullptr;
        TextRendererComponent* textRenderer = gameObject ? gameObject->GetComponent< TextRendererComponent >() : nullptr;
        DecalRendererComponent* decalRenderer = gameObject ? gameObject->GetComponent< DecalRendererComponent >() : nullptr;
        
        if (gameObject != nullptr && transform != nullptr)
        {
            nk_label( &ctx, "Position", NK_TEXT_LEFT );

            Vec3& pos = transform->GetLocalPosition();
            nk_layout_row_dynamic( &ctx, 40, 3 );

            //ctx.style.property.normal = nk_style_item_color( nk_rgb(216,6,6) );
            nk_property_float( &ctx, "#X:", -1024.0f, &pos.x, 1024.0f, 1, 1 );

            //ctx.style.property.normal = nk_style_item_color( nk_rgb(6,216,6) );    
            nk_property_float( &ctx, "#Y:", -1024.0f, &pos.y, 1024.0f, 1, 1 );

            //ctx.style.property.normal = nk_style_item_color( nk_rgb(6,6,216) );
            nk_property_float( &ctx, "#Z:", -1024.0f, &pos.z, 1024.0f, 1, 1 );

            /*nk_label( &ctx, "Rotation", NK_TEXT_LEFT );
            Quaternion& rot = transform->GetLocalRotation();
            Vec3 euler = rot.GetEuler();
            nk_layout_row_dynamic( &ctx, 40, 3 );
            nk_property_float( &ctx, "#X:", -1024.0f, &euler.x, 1024.0f, 1, 1 );
            nk_property_float( &ctx, "#Y:", -1024.0f, &euler.y, 1024.0f, 1, 1 );
            nk_property_float( &ctx, "#Z:", -1024.0f, &euler.z, 1024.0f, 1, 1 );*/
            //rot = Quaternion::FromEuler( euler );

            nk_layout_row_static( &ctx, 40, 200, 1 );
            float& scale = transform->GetLocalScale();
            nk_property_float( &ctx, "#Scale:", 0.001f, &scale, 1024.0f, 1, 1 );
            nk_layout_row_static( &ctx, 40, 450, 1 );
        }
        
        if (gameObject != nullptr && meshRenderer == nullptr && nk_button_label( &ctx, "Add mesh renderer" ))
        {
            gameObject->AddComponent< MeshRendererComponent >();
        }
        else if (gameObject != nullptr && meshRenderer != nullptr)
        {
            if (nk_tree_push( &ctx, NK_TREE_NODE, "Mesh Renderer", NK_MAXIMIZED ))
            {
                if (nk_button_label( &ctx, "Remove mesh renderer" ))
                {
                    gameObject->RemoveComponent< MeshRendererComponent >();
                }

                if (nk_button_label( &ctx, "Set mesh" ))
                {
                    Mesh* mesh = new Mesh;
                    char path[ 1024 ] = {};
                    GetOpenPath( path, "ae3d" );

                    if (strlen( path ) > 0)
                    {
                        mesh->Load( FileSystem::FileContents( path ) );
                        gameObject->GetComponent< MeshRendererComponent >()->SetMesh( mesh );

                        for (unsigned i = 0; i < mesh->GetSubMeshCount(); ++i)
                        {
                            gameObject->GetComponent< MeshRendererComponent >()->SetMaterial( material, i );
                        }
                    }
                }

                nk_tree_pop( &ctx );
                nk_spacing( &ctx, 1 );
            }
        }
        
        if (gameObject != nullptr && particleSystem == nullptr && nk_button_label( &ctx, "Add particle system" ))
        {
            gameObject->AddComponent< ParticleSystemComponent >();
        }
        else if (gameObject != nullptr && particleSystem != nullptr)
        {
            if (nk_tree_push( &ctx, NK_TREE_NODE, "Particle System", NK_MAXIMIZED ))
            {
                if (nk_button_label( &ctx, "Remove particle system" ))
                {
                    gameObject->RemoveComponent< ParticleSystemComponent >();
                    particleSystem = nullptr;
                }

                if (particleSystem)
                {
                    static nk_colorf color = { 1, 1, 1, 1 };
                    nk_color_pick( &ctx, &color, NK_RGB );
                    particleSystem->SetColor( color.r, color.g, color.b );

                    static int maxParticles = 10000;
                    nk_label( &ctx, "Max particles:", NK_TEXT_LEFT );
                    nk_slider_int( &ctx, 0, &maxParticles, 10000, 10 );
                    particleSystem->SetMaxParticles( maxParticles );
                }
                
                nk_tree_pop( &ctx );
                nk_spacing( &ctx, 1 );
            }
        }

        /*if (gameObject != nullptr && textRenderer == nullptr && nk_button_label( &ctx, "Add text renderer" ))
        {
            gameObject->AddComponent< TextRendererComponent >();
        }
        else if (gameObject != nullptr && textRenderer != nullptr)
        {
            textRenderer->SetText( "something something" );
            
            if (nk_tree_push( &ctx, NK_TREE_NODE, "Text Renderer", NK_MAXIMIZED ))
            {
                if (nk_button_label( &ctx, "Remove text renderer" ))
                {
                    gameObject->RemoveComponent< TextRendererComponent >();
                    textRenderer = nullptr;
                }

                nk_tree_pop( &ctx );
                nk_spacing( &ctx, 1 );
            }
            }*/

        if (gameObject != nullptr && audioSource == nullptr && nk_button_label( &ctx, "Add audio source" ))
        {
            gameObject->AddComponent< AudioSourceComponent >();
        }
        else if (gameObject != nullptr && audioSource != nullptr)
        {
            if (nk_tree_push( &ctx, NK_TREE_NODE, "Audio Source", NK_MAXIMIZED ))
            {
                if (nk_button_label( &ctx, "Remove audio source" ))
                {
                    gameObject->RemoveComponent< AudioSourceComponent >();
                    audioSource = nullptr;
                }

                if (audioSource && audioSource->GetClipId() == 0 && nk_button_label( &ctx, "Set audio clip" ))
                {
                    char path[ 1024 ] = {};
                    GetOpenPath( path, "wav" );

                    if (path[ 0 ] != '\0')
                    {
                        AudioClip* audioClip = new AudioClip;
                        audioClip->Load( FileSystem::FileContents( path ) );
                        gameObject->GetComponent< AudioSourceComponent >()->SetClipId( audioClip->GetId() );
                    }
                }

                if (audioSource && audioSource->GetClipId() != 0 && nk_button_label( &ctx, "Play" ))
                {
                    audioSource->Play();
                }

                nk_tree_pop( &ctx );
                nk_spacing( &ctx, 1 );
            }
        }
                
        if (gameObject != nullptr && camera == nullptr && nk_button_label( &ctx, "Add camera" ))
        {
            gameObject->AddComponent< CameraComponent >();
        }
        else if (gameObject != nullptr && camera != nullptr && nk_button_label( &ctx, "Remove camera" ))
        {
            gameObject->RemoveComponent< CameraComponent >();
            camera = nullptr;
        }

        if (gameObject != nullptr && pointLight == nullptr && nk_button_label( &ctx, "Add point light" ))
        {
            gameObject->AddComponent< PointLightComponent >();
        }

        if (gameObject != nullptr && pointLight != nullptr)
        {
            if (nk_tree_push( &ctx, NK_TREE_NODE, "Point Light", NK_MAXIMIZED ))
            {
                if (nk_button_label( &ctx, "Remove point light" ))
                {
                    gameObject->RemoveComponent< PointLightComponent >();
                    pointLight = nullptr;
                }

                if (pointLight)
                {
                    float& radius = pointLight->GetRadius();
                    nk_property_float( &ctx, "#Radius:", 0.0f, &radius, 1024.0f, 1, 1 );
                    
                    Vec3& color = pointLight->GetColor();
                    nk_layout_row_dynamic( &ctx, 40, 3 );
                    nk_property_float( &ctx, "#Red:", -1024.0f, &color.x, 1024.0f, 1, 1 );
                    nk_property_float( &ctx, "#Green:", -1024.0f, &color.y, 1024.0f, 1, 1 );
                    nk_property_float( &ctx, "#Blue:", -1024.0f, &color.z, 1024.0f, 1, 1 );
                    nk_layout_row_static( &ctx, 40, 450, 1 );

                    //static nk_colorf color2;
                    //nk_color_pick( &ctx, &color2, NK_RGB );
                }
                
                nk_tree_pop( &ctx );
                nk_spacing( &ctx, 1 );
            }
        }

        if (gameObject != nullptr && dirLight == nullptr && nk_button_label( &ctx, "Add directional light" ))
        {
            gameObject->AddComponent< DirectionalLightComponent >();
        }
        else if (gameObject != nullptr && dirLight != nullptr)
        {
            if (nk_tree_push( &ctx, NK_TREE_NODE, "Directional Light", NK_MAXIMIZED ))
            {
                if (nk_button_label( &ctx, "Remove directional light" ))
                {
                    gameObject->RemoveComponent< DirectionalLightComponent >();
                    dirLight = nullptr;
                }

                if (dirLight)
                {
                    int shadow = dirLight->CastsShadow();
                    nk_checkbox_label( &ctx, "Casts shadow", &shadow );
                
                    if (shadow == 1 && !dirLight->CastsShadow())
                    {
                        dirLight->SetCastShadow( true, 1024 );
                    }
                    else if (shadow == 0)
                    {
                        dirLight->SetCastShadow( false, 1024 );
                    }
                    
                    Vec3& color = dirLight->GetColor();
                    nk_layout_row_dynamic( &ctx, 40, 3 );
                    nk_property_float( &ctx, "#Red:", -1024.0f, &color.x, 1024.0f, 1, 1 );
                    nk_property_float( &ctx, "#Green:", -1024.0f, &color.y, 1024.0f, 1, 1 );
                    nk_property_float( &ctx, "#Blue:", -1024.0f, &color.z, 1024.0f, 1, 1 );
                    nk_layout_row_static( &ctx, 40, 450, 1 );
                }
                
                nk_tree_pop( &ctx );
                nk_spacing( &ctx, 1 );
            }
        }

        if (gameObject != nullptr && spotLight == nullptr && nk_button_label( &ctx, "Add spot light" ))
        {
            gameObject->AddComponent< SpotLightComponent >();
        }
        else if (gameObject != nullptr && spotLight != nullptr)
        {
            if (nk_tree_push( &ctx, NK_TREE_NODE, "Spot Light", NK_MAXIMIZED ))
            {
                if (nk_button_label( &ctx, "Remove spot light" ))
                {
                    gameObject->RemoveComponent< SpotLightComponent >();
                    spotLight = nullptr;
                }

                if (spotLight)
                {
                    int shadow = spotLight->CastsShadow();
                    nk_checkbox_label( &ctx, "Casts shadow", &shadow );

                    if (shadow == 1 && !spotLight->CastsShadow())
                    {
                        spotLight->SetCastShadow( true, 1024 );
                    }
                    else if (shadow == 0)
                    {
                        spotLight->SetCastShadow( false, 1024 );
                    }
                    
                    Vec3& color = spotLight->GetColor();
                    nk_layout_row_dynamic( &ctx, 40, 3 );
                    nk_property_float( &ctx, "#Red:", -1024.0f, &color.x, 1024.0f, 1, 1 );
                    nk_property_float( &ctx, "#Green:", -1024.0f, &color.y, 1024.0f, 1, 1 );
                    nk_property_float( &ctx, "#Blue:", -1024.0f, &color.z, 1024.0f, 1, 1 );
                    nk_layout_row_static( &ctx, 40, 450, 1 );

                    float& coneAngleDegrees = spotLight->GetConeAngle();
                    nk_property_float( &ctx, "#Cone angle:", -180.0f, &coneAngleDegrees, 180.0f, 45, 1 );
                }
                
                nk_tree_pop( &ctx );
                nk_spacing( &ctx, 1 );
            }
        }
        
        // Gameobject is not selected.
        
        if (gameObject == nullptr && nk_button_label( &ctx, "add game object" ))
        {
            outCommand = Command::CreateGO;
        }
        
        if (gameObject == nullptr && nk_button_label( &ctx, "open scene" ))
        {
            outCommand = Command::OpenScene;
        }

        if (gameObject == nullptr && nk_button_label( &ctx, "save scene" ))
        {
            outCommand = Command::SaveScene;
        }

        if (gameObject == nullptr)
        {
            static int ssa;
            nk_checkbox_label( &ctx, "SSAO", &ssa );
            ssao = ssa != 0;

            static int blo;
            nk_checkbox_label( &ctx, "Bloom", &blo );
            bloom = blo != 0;

            nk_label( &ctx, "Bloom threshold:", NK_TEXT_LEFT );
            nk_slider_float( &ctx, 0, &bloomThreshold, 1.0f, 0.05f );

            nk_label( &ctx, "Bloom intensity:", NK_TEXT_LEFT );
            nk_slider_float( &ctx, 0, &bloomIntensity, 10.0f, 0.05f );
        }
        
        // Hierarchy
        constexpr unsigned nameCount = 100;
        static const char* goNames[ nameCount ] = {};
        static int selected[ nameCount ] = {};

        for (unsigned i = 0; i < nameCount; ++i)
        {
            goNames[ i ] = " ";
        }

        for (unsigned i = 1; i < goCount; ++i)
        {
            if (i < nameCount)
            {
                goNames[ i - 1 ] = gameObjects[ i ]->GetName();
            }
        }

        if (gameObject == nullptr)
        {
            if (nk_tree_push( &ctx, NK_TREE_NODE, "Hierarchy", NK_MAXIMIZED ))
            {
                for (unsigned i = 0; i < goCount - 1; ++i)
                {
                    const char* name = strcmp( goNames[ i ], "" ) != 0 ? goNames[ i ] : "unnamed";
                    nk_selectable_label( &ctx, name, NK_TEXT_LEFT, &selected[ i ] );

                    if (selected[ i ] != 0)
                    {
                        outSelectedGameObject = i + 1;
                        selected[ i ] = 0;
                    }
                }

                nk_tree_pop( &ctx );
            }
        }
        
        DrawNuklear( width, height );

        nk_end( &ctx );
    }
}

void Inspector::Deinit()
{
    nk_font_atlas_clear( &atlas );
    nk_buffer_free( &uiCommands );
    nk_free( &ctx );
}
