#ifndef WINDOW_H
#define WINDOW_H

namespace ae3d
{
    enum class WindowEventType
    {
        None = 0,
        Close,
        KeyDown,
        KeyUp,
        Mouse1Down,
        Mouse1Up
    };

    enum WindowCreateFlags : unsigned
    {
        Empty = 1 << 0,
        Fullscreen = 1 << 1
    };

    struct WindowEvent
    {
        WindowEventType type = WindowEventType::None;
        int keyCode = 0;
    };
    
    class Window
    {
    public:
        static Window& Instance()
        {
            static Window instance;
            return instance;
        }

        /**
          Creates a window and rendering context.

          \param width Width in pixels or 0 to use desktop width.
          \param height Height in pixels or 0 to use desktop height.
          \param flags Bitmask of creation flags.
        */
        void Create( int width, int height, WindowCreateFlags flags );
        bool IsOpen();
        void PumpEvents();
        bool PollEvent( WindowEvent& outEvent );
        void SwapBuffers() const;

    private:
        Window() {}
        void CreateRenderer();
    };
}
#endif
