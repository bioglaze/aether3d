#include "Inspector.hpp"
#include "AudioSourceComponent.hpp"
#include "AudioClip.hpp"
#include "CameraComponent.hpp"
#include "DirectionalLightComponent.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "Mesh.hpp"
#include "MeshRendererComponent.hpp"
#include "PointLightComponent.hpp"
#include "SpotLightComponent.hpp"
#include "System.hpp"
#include "Texture2D.hpp"
#include "TransformComponent.hpp"
#include <string.h>
#include <stdio.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
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

void GetOpenPath( char* path );

struct VertexPTC
{
    float position[ 3 ];
    float uv[ 2 ];
    float col[ 4 ];
};

nk_draw_null_texture nullTexture;
Texture2D* uiTextures[ 1 ];
nk_context ctx;
nk_font_atlas atlas;
int atlasWidth = 0;
int atlasHeight = 0;
Texture2D nkFontTexture;

static unsigned Min2( unsigned a, unsigned b )
{
    return a < b ? a : b;
}

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
    
    nk_buffer vbuf, ebuf, uiCommands;
    nk_buffer_init_default( &uiCommands );
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

        System::DrawUI( (int)(cmd->clip_rect.x),
#if RENDERER_VULKAN
                        (int)(cmd->clip_rect.y - cmd->clip_rect.h),
#else
                        (int)((height - (int)(cmd->clip_rect.y + cmd->clip_rect.h))),
#endif
                        (int)(cmd->clip_rect.w),
                        (int)(cmd->clip_rect.h),
                        cmd->elem_count, uiTextures[ 0/*cmd->texture.id*/ ], offset, width, height );

        offset += cmd->elem_count / 3;
    }

    nk_clear( &ctx );
}

void Inspector::Init()
{    
    nk_font_atlas_init_default( &atlas );
    nk_font_atlas_begin( &atlas );

    nk_font* nkFont = nk_font_atlas_add_default( &atlas, 13.0f, nullptr );
    const void* image = nk_font_atlas_bake( &atlas, &atlasWidth, &atlasHeight, NK_FONT_ATLAS_RGBA32 );

#if RENDERER_VULKAN
    nkFontTexture.LoadFromData( image, atlasWidth, atlasHeight, 4, "Nuklear font", VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT );
#else
    nkFontTexture.LoadFromData( image, atlasWidth, atlasHeight, 4, "Nuklear font" );
#endif

    nk_font_atlas_end( &atlas, nk_handle_id( nkFontTexture.GetID() ), &nullTexture );
    
    uiTextures[ 0 ] = &nkFontTexture;
    
    nk_init_default( &ctx, &nkFont->handle );
}

void Inspector::BeginInput()
{
    nk_input_begin( &ctx );
}

void Inspector::EndInput()
{
    nk_input_end( &ctx );
}

void Inspector::HandleLeftMouseClick( int x, int y, int state )
{
    nk_input_button( &ctx, NK_BUTTON_LEFT, x, y, state );
}

void Inspector::HandleMouseMotion( int x, int y )
{
    nk_input_motion( &ctx, x, y );
}

void Inspector::Render( unsigned width, unsigned height, GameObject* gameObject, Command& outCommand, GameObject** gameObjects, unsigned goCount )
{
    outCommand = Command::Empty;

    if (nk_begin( &ctx, "Inspector", nk_rect( 0, 50, 300, 500 ), NK_WINDOW_BORDER | NK_WINDOW_TITLE ))
    {
        nk_layout_row_static( &ctx, 30, 150, 1 );

        // Gameobject is selected.

        if (gameObject != nullptr)
        {
            const char* str = gameObject->GetName().c_str();
            nk_label( &ctx, str, NK_TEXT_LEFT );
        }

        TransformComponent* transform = gameObject ? gameObject->GetComponent< TransformComponent >() : nullptr;
        MeshRendererComponent* meshRenderer = gameObject ? gameObject->GetComponent< MeshRendererComponent >() : nullptr;
        AudioSourceComponent* audioSource = gameObject ? gameObject->GetComponent< AudioSourceComponent >() : nullptr;
        CameraComponent* camera = gameObject ? gameObject->GetComponent< CameraComponent >() : nullptr;
        PointLightComponent* pointLight = gameObject ? gameObject->GetComponent< PointLightComponent >() : nullptr;
        SpotLightComponent* spotLight = gameObject ? gameObject->GetComponent< SpotLightComponent >() : nullptr;

        if (gameObject != nullptr && transform != nullptr)
        {
            Vec3 pos = transform->GetLocalPosition();
            
            nk_property_float( &ctx, "#X:", -1024.0f, &pos.x, 1024.0f, 1, 1 );
            nk_property_float( &ctx, "#Y:", -1024.0f, &pos.y, 1024.0f, 1, 1 );
            nk_property_float( &ctx, "#Z:", -1024.0f, &pos.z, 1024.0f, 1, 1 );
        }
        
        if (gameObject != nullptr && meshRenderer == nullptr && nk_button_label( &ctx, "Add mesh renderer" ))
        {
            gameObject->AddComponent< MeshRendererComponent >();
        }
        else if (gameObject != nullptr && meshRenderer != nullptr && nk_button_label( &ctx, "Remove mesh renderer" ))
        {
            gameObject->RemoveComponent< MeshRendererComponent >();
        }

        if (gameObject != nullptr && meshRenderer != nullptr && nk_button_label( &ctx, "Set mesh" ))
        {
            Mesh* mesh = new Mesh;
            char path[ 1024 ];
            GetOpenPath( path );
            mesh->Load( FileSystem::FileContents( path ) );
            gameObject->GetComponent< MeshRendererComponent >()->SetMesh( mesh );
        }

        if (gameObject != nullptr && audioSource == nullptr && nk_button_label( &ctx, "Add audio source" ))
        {
            gameObject->AddComponent< AudioSourceComponent >();
        }
        else if (gameObject != nullptr && audioSource != nullptr && nk_button_label( &ctx, "Remove audio source" ))
        {
            gameObject->RemoveComponent< AudioSourceComponent >();
        }

        if (gameObject != nullptr && audioSource != nullptr && nk_button_label( &ctx, "Set audio clip" ))
        {
            char path[ 1024 ];
            GetOpenPath( path );
            AudioClip* audioClip = new AudioClip;
            audioClip->Load( FileSystem::FileContents( path ) );
            gameObject->GetComponent< AudioSourceComponent >()->SetClipId( audioClip->GetId() );
        }

        if (gameObject != nullptr && camera == nullptr && nk_button_label( &ctx, "Add camera" ))
        {
            gameObject->AddComponent< CameraComponent >();
        }
        else if (gameObject != nullptr && camera != nullptr && nk_button_label( &ctx, "Remove camera" ))
        {
            gameObject->RemoveComponent< CameraComponent >();
        }

        if (gameObject != nullptr && pointLight == nullptr && nk_button_label( &ctx, "Add point light" ))
        {
            gameObject->AddComponent< PointLightComponent >();
        }
        else if (gameObject != nullptr && pointLight != nullptr && nk_button_label( &ctx, "Remove point light" ))
        {
            gameObject->RemoveComponent< PointLightComponent >();
        }

        if (gameObject != nullptr && pointLight != nullptr)
        {
            static float radius = 0;
            radius = pointLight->GetRadius();
            nk_property_float( &ctx, "#Radius:", -0.0f, &radius, 1024.0f, 1, 1 );
        }

        if (gameObject != nullptr && spotLight == nullptr && nk_button_label( &ctx, "Add spot light" ))
        {
            gameObject->AddComponent< SpotLightComponent >();
        }
        else if (gameObject != nullptr && spotLight != nullptr && nk_button_label( &ctx, "Remove spot light" ))
        {
            gameObject->RemoveComponent< SpotLightComponent >();
        }

        if (gameObject != nullptr && spotLight != nullptr)
        {
            const char* str = "testing";
            nk_label( &ctx, str, NK_TEXT_LEFT );
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

        // Hierarchy
        constexpr unsigned nameCount = 100;
        static const char* goNames[ nameCount ] = {};

        for (unsigned i = 1; i < goCount; ++i)
        {
            if (i < nameCount)
            {
                goNames[ i - 1 ] = gameObjects[ i ]->GetName().c_str();
            }
        }

        if (gameObject == nullptr)
        {
            nk_label( &ctx, "Hierarchy", NK_TEXT_LEFT );
            static int currentGo = 0;
            const unsigned itemCount = Min2( nameCount, goCount - 1 );
            currentGo = nk_combo( &ctx, goNames, itemCount, currentGo, 25, nk_vec2( nk_widget_width( &ctx ), 200 ) );
        }
        
        DrawNuklear( width, height );
        nk_end( &ctx );
    }
}

void Inspector::Deinit()
{
    nk_font_atlas_clear( &atlas );
    nk_free( &ctx );
}
