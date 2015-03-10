namespace ae3d
{
    enum class WindowEventType { Close = 0 };

    struct WindowEvent
    {
        WindowEventType type;
    };
    
    class Window
    {
    public:
        static Window& Instance()
        {
            static Window instance;
            return instance;
        }

        void Create(int width, int height );
        void CreateRenderer();
        bool IsOpen();
        void PumpEvents();
        bool PollEvent( WindowEvent& outEvent );
        
    private:
        Window() {}
    };
}

