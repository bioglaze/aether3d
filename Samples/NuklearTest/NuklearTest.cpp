#include <string.h>
#include "CameraComponent.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "Scene.hpp"
#include "System.hpp"
#include "Texture2D.hpp"
#include "TransformComponent.hpp"
#include "Vec3.hpp"
#include "Window.hpp"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include "nuklear.h"

using namespace ae3d;

struct VertexPTC
{
    float position[ 3 ];
    float uv[ 2 ];
    float col[ 4 ];
};

nk_draw_null_texture nullTexture;
Texture2D* uiTextures[ 1 ];

void DrawNuklear( nk_context* ctx, nk_buffer* uiCommands, int width, int height )
{
    struct nk_convert_config config;
    static const struct nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(VertexPTC, position)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(VertexPTC, uv)},
        {NK_VERTEX_COLOR, NK_FORMAT_R32G32B32A32_FLOAT, NK_OFFSETOF(VertexPTC, col)},
        {NK_VERTEX_LAYOUT_END}
    };

    NK_MEMSET( &config, 0, sizeof( config ) );
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
    nk_buffer_init_fixed( &vbuf, vertices, MAX_VERTEX_MEMORY );
    nk_buffer_init_fixed( &ebuf, elements, MAX_ELEMENT_MEMORY );
    nk_convert( ctx, uiCommands, &vbuf, &ebuf, &config );
#if RENDERER_VULKAN
    for (int i = 0; i < MAX_VERTEX_MEMORY / sizeof( VertexPTC ); ++i)
    {
        VertexPTC* vert = &((VertexPTC*)vertices)[ i ];
        vert->position[ 1 ] = height - vert->position[ 1 ];
    }
#endif

    System::UnmapUIVertexBuffer();
    
    const struct nk_draw_command* cmd = nullptr;
    nk_draw_index* offset = nullptr;

    nk_draw_foreach( cmd, ctx, uiCommands )
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

    nk_clear( ctx );
}

int main()
{
    int width = 640;
    int height = 480;
    
    System::EnableWindowsMemleakDetection();
    Window::Create( width, height, WindowCreateFlags::Empty );
    Window::GetSize( width, height );
    System::LoadBuiltinAssets();

    GameObject camera;
    camera.AddComponent<CameraComponent>();
    camera.GetComponent<CameraComponent>()->SetProjection( 0, (float)width, (float)height, 0, 0, 1 );
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0.1f, 0.1f, 0.1f ) );
    camera.GetComponent<CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::DepthAndColor );
    camera.AddComponent<TransformComponent>();
   
    Scene scene;
    scene.Add( &camera );

    Texture2D gliderTexture;
    gliderTexture.Load( FileSystem::FileContents("glider.png"), TextureWrap::Repeat, TextureFilter::Nearest, Mipmaps::Generate, ColorSpace::RGB, Anisotropy::k1 );

    bool quit = false;

    nk_context ctx;
    nk_font_atlas atlas;
    int atlasWidth = 0;
    int atlasHeight = 0;
    Texture2D nkFontTexture;
    nk_buffer cmds;
    
    nk_font_atlas_init_default( &atlas );
    nk_font_atlas_begin( &atlas );

    nk_font* nkFont = nk_font_atlas_add_default( &atlas, 13.0f, nullptr );
    const void* image = nk_font_atlas_bake( &atlas, &atlasWidth, &atlasHeight, NK_FONT_ATLAS_RGBA32 );

    nkFontTexture.LoadFromData( image, atlasWidth, atlasHeight, 4, "Nuklear font" );
    nk_font_atlas_end( &atlas, nk_handle_id( nkFontTexture.GetID() ), &nullTexture );
    
    uiTextures[ 0 /*nk_handle_id( nkFontTexture.GetID() ).id*/ ] = &nkFontTexture;
    
    nk_init_default( &ctx, &nkFont->handle );
    nk_buffer_init_default( &cmds );
   
    double x = 0, y = 0;
    
    int winWidth, winHeight;
    Window::GetSize( winWidth, winHeight );
    System::Print( "window size: %dx%d\n", winWidth, winHeight );

    while (Window::IsOpen() && !quit)
    {
        Window::PumpEvents();
        WindowEvent event;

        nk_input_begin( &ctx );

        while (Window::PollEvent( event ))
        {
            if (event.type == WindowEventType::Close)
            {
                quit = true;
            }
            
            if (event.type == WindowEventType::KeyDown ||
                event.type == WindowEventType::KeyUp)
            {
                KeyCode keyCode = event.keyCode;

                if (keyCode == KeyCode::Escape)
                {
                    quit = true;
                }
                else if (keyCode == KeyCode::A)
                {
                    nk_input_char( &ctx, 'a' );
                }
                else if (keyCode == KeyCode::K)
                {
                    nk_input_char( &ctx, 'k' );
                }
                else if (keyCode == KeyCode::Left)
                {
                    nk_input_key( &ctx, NK_KEY_LEFT, event.type == WindowEventType::KeyDown );
                }
                else if (keyCode == KeyCode::Right)
                {
                    nk_input_key( &ctx, NK_KEY_RIGHT, event.type == WindowEventType::KeyDown );
                }
            }

            if (event.type == WindowEventType::MouseMove)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                nk_input_motion( &ctx, (int)x, (int)y );
            }

            if (event.type == WindowEventType::Mouse1Down)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                nk_input_button( &ctx, NK_BUTTON_LEFT, (int)x, (int)y, 1 );
            }

            if (event.type == WindowEventType::Mouse1Up)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                nk_input_button( &ctx, NK_BUTTON_LEFT, (int)x, (int)y, 0 );
            }
            
            //nk_input_button( &ctx, NK_BUTTON_RIGHT, (int)x, (int)y, (event.type == WindowEventType::Mouse2Down) ? 1 : 0 );
        }

        nk_input_end( &ctx );

        static float value = 0.6f;
        
        if (nk_begin( &ctx, "Demo", nk_rect( 0, 50, 300, 400 ), NK_WINDOW_BORDER | NK_WINDOW_TITLE ))
        {
            nk_layout_row_static( &ctx, 30, 80, 1 );
            
            if (nk_button_label( &ctx, "button" ))
            {
                System::Print("Pressed a button\n");
            }

            nk_layout_row_dynamic( &ctx, 30, 2 );
            enum { EASY, HARD };
            static int op = EASY;
            if (nk_option_label( &ctx, "easy", op == EASY )) op = EASY;
            if (nk_option_label( &ctx, "hard", op == HARD )) op = HARD;
            nk_check_label( &ctx, "check1", 0 );
            nk_check_label( &ctx, "check2", 1 );
            static size_t prog_value = 54;
            nk_progress( &ctx, &prog_value, 100, NK_MODIFIABLE );

            static char field_buffer[ 64 ];
            static int field_len;
            nk_edit_string( &ctx, NK_EDIT_FIELD, field_buffer, &field_len, 64, nk_filter_default );
            static float pos;

            nk_property_float( &ctx, "#X:", -1024.0f, &pos, 1024.0f, 1, 1 );

            static const char *weapons[] = {"Fist","Pistol","Shotgun" };
            static int current_weapon = 0;
            current_weapon = nk_combo( &ctx, weapons, 3, current_weapon, 25, nk_vec2( nk_widget_width( &ctx ), 200 ) );

            nk_layout_row_begin( &ctx, NK_STATIC, 30, 2 );
            {
                nk_layout_row_push( &ctx, 50 );
                nk_label( &ctx, "Volume:", NK_TEXT_LEFT );
                nk_layout_row_push( &ctx, 110 );
                nk_slider_float( &ctx, 0, &value, 1.0f, 0.1f );
            }
            nk_layout_row_end( &ctx );
            
            nk_layout_row_dynamic( &ctx, 250, 1 );
            if (nk_group_begin( &ctx, "Standard", NK_WINDOW_BORDER ))
            {
                if (nk_tree_push(&ctx, NK_TREE_NODE, "Window", NK_MAXIMIZED))
                {
                    static int selected[ 8 ];
                    
                    if (nk_tree_push( &ctx, NK_TREE_NODE, "Next", NK_MAXIMIZED ))
                    {
                        nk_layout_row_dynamic( &ctx, 20, 1 );
                        
                        for (int i = 0; i < 4; ++i)
                        {
                            nk_selectable_label( &ctx, (selected[ i ]) ? "Selected": "Unselected", NK_TEXT_LEFT, &selected[ i ] );
                        }
                        
                        nk_tree_pop( &ctx );
                    }
                    if (nk_tree_push( &ctx, NK_TREE_NODE, "Previous", NK_MAXIMIZED ))
                    {
                        nk_layout_row_dynamic( &ctx, 20, 1 );
                        
                        for (int i = 4; i < 8; ++i)
                        {
                            nk_selectable_label( &ctx, (selected[i]) ? "Selected": "Unselected", NK_TEXT_LEFT, &selected[i] );
                        }
                        
                        nk_tree_pop( &ctx );
                    }
                    
                    nk_tree_pop( &ctx );
                }
                
                nk_group_end( &ctx );
            }
        
            nk_end( &ctx );
        }

        scene.Render();
        DrawNuklear( &ctx, &cmds, width, height );
        scene.EndFrame();

        Window::SwapBuffers();
    }

	nk_font_atlas_clear( &atlas );
	nk_free( &ctx );

    System::Deinit();
	return 0;
}
