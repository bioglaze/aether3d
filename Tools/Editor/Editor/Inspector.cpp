#include "Inspector.hpp"
#include "System.hpp"
#include "Texture2D.hpp"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include "../../Samples/NuklearTest/nuklear.h"

using namespace ae3d;

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
nk_buffer cmds;

void DrawNuklear( nk_context* ctx, nk_buffer* uiCommands, int width, int height )
{
    struct nk_convert_config config;
    static const struct nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct VertexPTC, position)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct VertexPTC, uv)},
        {NK_VERTEX_COLOR, NK_FORMAT_R32G32B32A32_FLOAT, NK_OFFSETOF(struct VertexPTC, col)},
        {NK_VERTEX_LAYOUT_END}
    };

    NK_MEMSET( &config, 0, sizeof( config ) );
    config.vertex_layout = vertex_layout;
    config.vertex_size = sizeof( struct VertexPTC );
    config.vertex_alignment = NK_ALIGNOF( struct VertexPTC );
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
    
    nk_buffer vbuf, ebuf;
    nk_buffer_init_fixed( &vbuf, vertices, MAX_VERTEX_MEMORY );
    nk_buffer_init_fixed( &ebuf, elements, MAX_ELEMENT_MEMORY );
    nk_convert( ctx, uiCommands, &vbuf, &ebuf, &config );

    System::UnmapUIVertexBuffer();
    
    const struct nk_draw_command* cmd = nullptr;
    nk_draw_index* offset = nullptr;

    const float scaleX = 1;
    const float scaleY = 1;
    
    nk_draw_foreach( cmd, ctx, uiCommands )
    {
        if (cmd->elem_count == 0)
        {
            continue;
        }
        
        System::DrawUI( (int)(cmd->clip_rect.x * scaleX),
                        (int)((height - (int)(cmd->clip_rect.y + cmd->clip_rect.h)) * scaleY),
                        (int)(cmd->clip_rect.w * scaleX),
                        (int)(cmd->clip_rect.h * scaleY),
                        cmd->elem_count, uiTextures[ cmd->texture.id ], offset, width, height );
        offset += cmd->elem_count;
    }

    nk_clear( ctx );
}

void Inspector::Init()
{    
    nk_font_atlas_init_default( &atlas );
    nk_font_atlas_begin( &atlas );

    nk_font* nkFont = nk_font_atlas_add_default( &atlas, 13.0f, nullptr );
    const void* image = nk_font_atlas_bake( &atlas, &atlasWidth, &atlasHeight, NK_FONT_ATLAS_RGBA32 );

    nkFontTexture.LoadFromData( image, atlasWidth, atlasHeight, 4, "Nuklear font" );
    nk_font_atlas_end( &atlas, nk_handle_id( nkFontTexture.GetID() ), &nullTexture );
    
    uiTextures[ 0 ] = &nkFontTexture;
    
    nk_init_default( &ctx, &nkFont->handle );
    nk_buffer_init_default( &cmds );

}

void Inspector::Render( int width, int height )
{
    if (nk_begin( &ctx, "Demo", nk_rect( 0, 50, 300, 400 ), NK_WINDOW_BORDER | NK_WINDOW_TITLE ))
    {
        nk_layout_row_static( &ctx, 30, 80, 1 );
            
        if (nk_button_label( &ctx, "button" ))
        {
            System::Print("Pressed a button\n");
        }

        DrawNuklear( &ctx, &cmds, width, height );
        nk_end( &ctx );
    }
}
