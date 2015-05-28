#ifndef WINDOW_H
#define WINDOW_H

namespace ae3d
{
    /// Window events.
    enum class WindowEventType
    {
        None = 0,
        Close,
        KeyDown,
        KeyUp,
        Mouse1Down,
        Mouse1Up,
        Mouse2Down,
        Mouse2Up,
        MouseMiddleDown,
        MouseMiddleUp,
        MouseWheelScrollDown,
        MouseWheelScrollUp,
        GamePadButtonA,
        GamePadButtonB,
        GamePadButtonX,
        GamePadButtonY,
        GamePadButtonDPadUp,
        GamePadButtonDPadDown,
        GamePadButtonDPadLeft,
        GamePadButtonDPadRight,
        GamePadButtonStart,
        GamePadButtonBack,
        GamePadButtonLeftShoulder,
        GamePadButtonRightShoulder,
        GamePadLeftThumbState,
        GamePadRightThumbState,
    };

    /// Key codes.
    enum class KeyCode
    {
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Left, Right, Up, Down, Space, Escape, Enter
    };
    
    /// Create flags.
    enum WindowCreateFlags : unsigned
    {
        Empty = 1 << 0,
        Fullscreen = 1 << 1
    };

    /// Window event is a key press, close event, mouse click etc.
    struct WindowEvent
    {
        /// Event type.
        WindowEventType type = WindowEventType::None;
        /// Key code.
        KeyCode keyCode = KeyCode::A;
        /// X coordinate in window.
        int mouseX = 0; 
        /// Y coordinate in window.
        int mouseY = 0;
        /// Gamepad's thumb x in range [-1, 1]. Event type indicates left or right thumb.
        float gamePadThumbX = 0;
        /// Gamepad's thumb y in range [-1, 1]. Event type indicates left or right thumb.
        float gamePadThumbY = 0;
    };

    /// Window singleton.    
    class Window
    {
    public:
        /// \return the window instance.
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

        /// \return True if window is open.
        bool IsOpen();

        /// Reads events from windowing system to be used in PollEvent depending on platform.
        void PumpEvents();

        /// Reads and discards the most recent event.
        /// \return True if there are more events.
        bool PollEvent( WindowEvent& outEvent );

        /// Displays the contents of the screen.
        void SwapBuffers() const;

    private:
        Window() {}
        void CreateRenderer();
    };
}
#endif
