#include "Window.hpp"

namespace ae3d
{
    void Window::Create( int width, int height, WindowCreateFlags flags )
    {
    }

    bool Window::PollEvent(WindowEvent& outEvent)
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

    void Window::SwapBuffers() const
    {
    }
}