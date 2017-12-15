#include <map>
#include <iostream>
#include <string>
#include "CameraComponent.hpp"
#include "TransformComponent.hpp"
#include "GameObject.hpp"
#include "Scene.hpp"
#include "System.hpp"
#include "FileSystem.hpp"
#include "Vec3.hpp"
#include "Window.hpp"
#include "Texture2D.hpp"

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
std::map< int, Texture2D* > uiTextures;

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

    const float scaleX = 2;
    const float scaleY = 2;
    
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
    int frame = 0;

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
    
    uiTextures[ nk_handle_id( nkFontTexture.GetID() ).id ] = &nkFontTexture;
    
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
                if (keyCode == KeyCode::A)
                {
                    System::ReloadChangedAssets();
                }
            }

            if (event.type == WindowEventType::MouseMove)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                //std::cout << "mouse position: " << x << ", y: " << y << std::endl;
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

        enum { EASY, HARD };
        static int op = EASY;
        static float value = 0.6f;
        
        if (nk_begin( &ctx, "Demo", nk_rect( 0, 50, 300, 400 ), NK_WINDOW_BORDER | NK_WINDOW_TITLE ))
        {
            nk_layout_row_static( &ctx, 30, 80, 1 );
            
            if (nk_button_label( &ctx, "button" ))
            {
                System::Print("Pressed a button\n");
            }

            /* fixed widget window ratio width */
            nk_layout_row_dynamic( &ctx, 30, 2 );
            if (nk_option_label( &ctx, "easy", op == EASY )) op = EASY;
            if (nk_option_label( &ctx, "hard", op == HARD )) op = HARD;

            /* custom widget pixel width */
            nk_layout_row_begin( &ctx, NK_STATIC, 30, 2 );
            {
                nk_layout_row_push( &ctx, 50 );
                nk_label( &ctx, "Volume:", NK_TEXT_LEFT );
                nk_layout_row_push( &ctx, 110 );
                nk_slider_float( &ctx, 0, &value, 1.0f, 0.1f );
            }
            nk_layout_row_end( &ctx );
            nk_end( &ctx );
        }

        scene.Render();
        DrawNuklear( &ctx, &cmds, width, height );
        scene.EndFrame();

        Window::SwapBuffers();

        ++frame;
    }

    System::Deinit();
}
