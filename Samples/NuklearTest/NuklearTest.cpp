#include <string>
#include "Font.hpp"
#include "CameraComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "TextRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "GameObject.hpp"
#include "Scene.hpp"
#include "System.hpp"
#include "FileSystem.hpp"
#include "Vec3.hpp"
#include "Window.hpp"
#include "Texture2D.hpp"
#include "Shader.hpp"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include "nuklear.h"

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

using namespace ae3d;

struct nk_glfw_vertex
{
    float position[ 2 ];
    float uv[ 2 ];
    nk_byte col[ 4 ];
};

nk_draw_null_texture nullTexture;

// Sample assets can be downloaded from here: http://twiren.kapsi.fi/files/aether3d_sample_v0.7.zip
// Extract them into aether3d_build that is generated next to aether3d folder.

void DrawNuklear( nk_context* ctx, nk_buffer* uiCommands, int width, int height )
{
    struct nk_convert_config config;
    static const struct nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, position)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, uv)},
        {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_glfw_vertex, col)},
        {NK_VERTEX_LAYOUT_END}
    };
    NK_MEMSET( &config, 0, sizeof( config ) );
    config.vertex_layout = vertex_layout;
    config.vertex_size = sizeof( struct nk_glfw_vertex );
    config.vertex_alignment = NK_ALIGNOF( struct nk_glfw_vertex );
    config.null = nullTexture;
    config.circle_segment_count = 22;
    config.curve_segment_count = 22;
    config.arc_segment_count = 22;
    config.global_alpha = 1.0f;
    config.shape_AA = NK_ANTI_ALIASING_ON;
    config.line_AA = NK_ANTI_ALIASING_ON;

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
                        cmd->elem_count, cmd->texture.id, offset );
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
    camera.GetComponent<CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::DontClear );
    camera.AddComponent<TransformComponent>();
   
    Texture2D spriteTex;
    spriteTex.Load( FileSystem::FileContents( "glider.png" ), TextureWrap::Repeat, TextureFilter::Nearest, Mipmaps::None, ColorSpace::RGB, Anisotropy::k1 );

    GameObject spriteContainer;
    spriteContainer.AddComponent<SpriteRendererComponent>();
    spriteContainer.AddComponent<TransformComponent>();
    auto sprite = spriteContainer.GetComponent<SpriteRendererComponent>();
    sprite->SetTexture( &spriteTex, Vec3( 320, 0, -0.6f ), Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), Vec4( 1, 0.5f, 0.5f, 1 ) );
    
    Texture2D fontTex;
    fontTex.Load( FileSystem::FileContents( "font.png" ), TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::RGB, Anisotropy::k1 );
    
    Font font;
    font.LoadBMFont( &fontTex, FileSystem::FileContents( "font_txt.fnt" ) );

    GameObject textContainer;
    textContainer.AddComponent<TextRendererComponent>();
    textContainer.GetComponent<TextRendererComponent>()->SetText( "Aether3D \nGame Engine" );
    textContainer.GetComponent<TextRendererComponent>()->SetFont( &font );
                                                                       
    Scene scene;
    //scene.Add( &camera );
    //scene.Add( &spriteContainer );
    //scene.Add( &textContainer );
    sprite->SetTexture( &spriteTex, Vec3( 420, 0, -0.6f ), Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), Vec4( 1, 0.5f, 0.5f, 1 ) );

    scene.Add( &camera );
    //scene.Add( &spriteContainer );
    //scene.Add( &textContainer );
    sprite->SetTexture( &spriteTex, Vec3( 420, 0, -0.6f ), Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), Vec4( 1, 0.5f, 0.5f, 1 ) );
    camera.GetComponent<CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::DepthAndColor );

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
    
    nk_init_default( &ctx, &nkFont->handle );
    nk_buffer_init_default( &cmds );
   
    double x = 0, y = 0;

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
                nk_input_motion( &ctx, (int)x, (int)y );
            }

            nk_input_button( &ctx, NK_BUTTON_LEFT, (int)x, (int)y, (event.type == WindowEventType::Mouse1Down) ? 1 : 1 );
            nk_input_button( &ctx, NK_BUTTON_RIGHT, (int)x, (int)y, (event.type == WindowEventType::Mouse2Down) ? 1 : 0 );
        }

        nk_input_end( &ctx );

        enum {EASY, HARD};
        static int op = EASY;
        static float value = 0.6f;
        
        if (nk_begin( &ctx, "Demo", nk_rect( 0, 50, 300, 400 ), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE ))
        {
            nk_layout_row_static( &ctx, 30, 120, 1 );
            
            if (nk_button_label( &ctx, "button" ))
            {
                ae3d::System::Print( "Pressed a button\n" );
            }
#if 1
            nk_layout_row_static( &ctx, 30, 80, 1 );
            if (nk_button_label( &ctx, "button" )) {
                /* event handling */
                System::Print("Pressed a button\n");
            }

            /* fixed widget window ratio width */
            nk_layout_row_dynamic(&ctx, 30, 2);
            if (nk_option_label(&ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(&ctx, "hard", op == HARD)) op = HARD;

            /* custom widget pixel width */
            nk_layout_row_begin(&ctx, NK_STATIC, 30, 2);
            {
                nk_layout_row_push(&ctx, 50);
                nk_label(&ctx, "Volume:", NK_TEXT_LEFT);
                nk_layout_row_push(&ctx, 110);
                nk_slider_float(&ctx, 0, &value, 1.0f, 0.1f);
            }
            nk_layout_row_end( &ctx );
#endif
            nk_end( &ctx );
        }

        scene.Render();
        DrawNuklear( &ctx, &cmds, width, height );
        
        Window::SwapBuffers();

        ++frame;
        textContainer.GetComponent<TextRendererComponent>()->SetText( (frame % 5 == 0) ? "Aether3D \nGame Engine" : "Aether3D" );
        textContainer.GetComponent<TextRendererComponent>()->SetText( System::Statistics::GetStatistics().c_str() );
    }

    System::Deinit();
}
