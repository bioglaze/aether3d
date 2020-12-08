#pragma once

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
        MouseMove,
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
        Left, Right, Up, Down, Space, Escape, Enter, Delete, Backspace, Dot, N0, N1, N2, N3, N4, N5, N6, N7, N8, N9, PageUp, PageDown, Empty
    };

    /// Key modifiers.
    enum class KeyModifier
    {
        Empty, Shift, Control
    };
    
    /// Create flags.
    enum WindowCreateFlags : unsigned
    {
        Empty = 1 << 0,
        Fullscreen = 1 << 1,
        MSAA4 = 1 << 2,
        MSAA8 = 1 << 3,
        MSAA16 = 1 << 4,
        No_vsync = 1 << 5
    };

    /// Window event is a key press, close event, mouse click etc.
    struct WindowEvent
    {
        /// Event type.
        WindowEventType type = WindowEventType::None;
        /// Key code.
        KeyCode keyCode = KeyCode::Empty;
        /// Key modifier.
        KeyModifier keyModifier = KeyModifier::Empty;
        /// X coordinate in window. Origin: left.
        int mouseX = 0; 
        /// Y coordinate in window. Origin: bottom.
        int mouseY = 0;
        /// Gamepad's thumb x in range [-1, 1]. Event type indicates left or right thumb.
        float gamePadThumbX = 0;
        /// Gamepad's thumb y in range [-1, 1]. Event type indicates left or right thumb.
        float gamePadThumbY = 0;
        // Mouse wheel.
        int mouseWheel = 0;
    };

    /// Window
    namespace Window
    {
        /// \return true, if the system is running on Wayland.
        bool IsWayland();
        
        /**
          Creates a window and rendering context.

          \param width Width in pixels or 0 to use desktop width.
          \param height Height in pixels or 0 to use desktop height.
          \param flags Bitmask of creation flags.
        */
        void Create( int width, int height, WindowCreateFlags flags );

        /// \param outWidth Returns window width in pixels.
        /// \param outHeight Returns window height in pixels, without the title bar.
        void GetSize( int& outWidth, int& outHeight );

        /// \return True if window is open.
        bool IsOpen();

        /// \return True, if key is down.
        bool IsKeyDown( KeyCode keyCode );

        /// Reads events from windowing system to be used in PollEvent depending on platform.
        void PumpEvents();

        /// Reads and discards the most recent event.
        /// \return True if there are more events.
        bool PollEvent( WindowEvent& outEvent );

        /// \param title Title.
        void SetTitle( const char* title );
        
        /// Displays the contents of the screen.
        void SwapBuffers();
    }
}
