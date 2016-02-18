#include "Window.hpp"

void PlatformInitGamePad()
{
}

namespace ae3d
{
    void Window::Create( int width, int height, WindowCreateFlags flags )
    {
    }

    bool Window::PollEvent( WindowEvent& outEvent )
    {
        return true;
    }

    void Window::PumpEvents()
    {
    }

    bool Window::IsOpen()
    {
        return true;
    }

    void Window::SwapBuffers()
    {
    }
    
    void Window::GetSize( int& outWidth, int& outHeight )
    {
    }
    
    void Window::SetTitle( const char* title )
    {
    }
}