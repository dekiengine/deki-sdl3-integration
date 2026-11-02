#include "SDL3GLDisplay.h"

#if defined(DEKI_PACKAGE_GL) && !defined(DEKI_EDITOR)

#include <deki/LogSystem.h>

namespace DekiSdl3
{

SDL3GLDisplay::~SDL3GLDisplay()
{
    Shutdown();
}

bool SDL3GLDisplay::Initialize(int32_t width, int32_t height)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        DEKI_LOG_ERROR("SDL3GLDisplay: SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    // OpenGL ES 3.0 on Linux, the API a handheld has, so the desktop runs the
    // same shaders; desktop OpenGL 3.3 core elsewhere, which Windows and
    // macOS drivers offer.
#if defined(__linux__)
    const bool es = true;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    const bool es = false;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    // The window only receives the finished frame: depth lives in the
    // presenter's own framebuffer.
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

    m_Window = SDL_CreateWindow("Deki", width * m_WindowScale, height * m_WindowScale, SDL_WINDOW_OPENGL);
    if (!m_Window)
    {
        DEKI_LOG_WARNING("SDL3GLDisplay: no OpenGL window: %s", SDL_GetError());
        Shutdown();
        return false;
    }
    m_GLContext = SDL_GL_CreateContext(m_Window);
    if (!m_GLContext || !SDL_GL_MakeCurrent(m_Window, m_GLContext))
    {
        DEKI_LOG_WARNING("SDL3GLDisplay: no OpenGL %s context: %s", es ? "ES 3.0" : "3.3", SDL_GetError());
        Shutdown();
        return false;
    }
    // As SDL3Display: never wait for the vertical blank; the engine paces.
    SDL_GL_SetSwapInterval(0);

    DekiGL::GLContext::Host host;
    host.load = [](const char* name) { return reinterpret_cast<void*>(SDL_GL_GetProcAddress(name)); };
    host.makeCurrent = [](void* user)
    {
        auto* self = static_cast<SDL3GLDisplay*>(user);
        return SDL_GL_GetCurrentContext() == self->m_GLContext || SDL_GL_MakeCurrent(self->m_Window, self->m_GLContext);
    };
    host.user = this;
    host.isES = es;
    std::unique_ptr<DekiGL::GLContext> adopted = DekiGL::GLContext::Adopt(host);
    m_Context = std::move(adopted);

    DekiGL::GLPresenter::Window window;
    window.swap = [](void* user) { SDL_GL_SwapWindow(static_cast<SDL3GLDisplay*>(user)->m_Window); };
    window.drawableSize = [](void* user, int* w, int* h)
    { SDL_GetWindowSizeInPixels(static_cast<SDL3GLDisplay*>(user)->m_Window, w, h); };
    window.user = this;
    m_Presenter = m_Context ? DekiGL::GLPresenter::Create(m_Context, window) : nullptr;
    if (!m_Presenter)
    {
        DEKI_LOG_WARNING("SDL3GLDisplay: OpenGL came up but cannot present; falling back");
        Shutdown();
        return false;
    }

    m_Presenter->SetDraws2D(m_Draw2DOnGpu);

    m_Width = width;
    m_Height = height;
    DEKI_LOG_INFO("SDL3GLDisplay: %dx%d screen in a x%d window, presented through OpenGL", width, height,
                  m_WindowScale);
    return true;
}

void SDL3GLDisplay::Shutdown()
{
    // The presenter first, while its context is still there to free in; then
    // whoever still holds the context learns it is gone.
    m_Presenter.reset();
    if (m_Context)
    {
        m_Context->MarkLost();
        m_Context.reset();
    }
    if (m_GLContext)
    {
        SDL_GL_DestroyContext(m_GLContext);
        m_GLContext = nullptr;
    }
    if (m_Window)
    {
        SDL_DestroyWindow(m_Window);
        m_Window = nullptr;
    }
}

void SDL3GLDisplay::Present(const uint8_t* framebuffer, int width, int height, Deki::ColorFormat format)
{
    if (m_Presenter)
    {
        m_Presenter->Present(framebuffer, width, height, format);
    }
}

void SDL3GLDisplay::GetDisplaySize(int32_t* width, int32_t* height) const
{
    *width = m_Width;
    *height = m_Height;
}

}  // namespace DekiSdl3

#endif
