#include "SDL3Input.h"
#include "Keys.h"  // from deki-input

#include <deki/Engine.h>
#include <deki/providers/IRenderSystem.h>

namespace DekiSdl3
{

namespace
{
// Mouse events arrive in window pixels, but the engine's screen-to-world math
// expects framebuffer pixels. The window is larger than the framebuffer by
// windowScale, so coordinates are scaled down before dispatch. Window and
// framebuffer share an aspect ratio, so a straight ratio is exact, with no
// letterbox offset.
void WindowToFramebuffer(SDL_WindowID windowID, float& x, float& y)
{
    SDL_Window* window = SDL_GetWindowFromID(windowID);
    if (!window)
    {
        return;
    }
    int winW = 0, winH = 0;
    SDL_GetWindowSize(window, &winW, &winH);
    if (winW <= 0 || winH <= 0)
    {
        return;
    }

    Deki::IRenderSystem* rs = Deki::Engine::GetInstance().GetRenderSystem();
    if (!rs)
    {
        return;
    }
    int32_t fbW = rs->GetScreenWidth();
    int32_t fbH = rs->GetScreenHeight();
    if (fbW <= 0 || fbH <= 0)
    {
        return;
    }

    x *= static_cast<float>(fbW) / static_cast<float>(winW);
    y *= static_cast<float>(fbH) / static_cast<float>(winH);
}
}  // namespace

SDL3Input::SDL3Input()
    : initialized(false),
      m_QuitFlag(false),
      m_MouseX(0),
      m_MouseY(0),
      m_MousePressed(false)
{
}

SDL3Input::~SDL3Input()
{
    Shutdown();
}

bool SDL3Input::Initialize()
{
    if (initialized)
    {
        return true;
    }
    initialized = true;
    m_QuitFlag = false;
    return true;
}

void SDL3Input::Shutdown()
{
    initialized = false;
}

void SDL3Input::Update()
{
    if (!initialized)
    {
        return;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ProcessSDLEvent(event);
    }

    // Update mouse state (scale window pixels down to framebuffer pixels)
    float mx, my;
    SDL_MouseButtonFlags mouseState = SDL_GetMouseState(&mx, &my);
    if (SDL_Window* focus = SDL_GetMouseFocus())
    {
        WindowToFramebuffer(SDL_GetWindowID(focus), mx, my);
    }
    m_MouseX = (int32_t)mx;
    m_MouseY = (int32_t)my;
    m_MousePressed = (mouseState & SDL_BUTTON_LMASK) != 0;
}

void SDL3Input::ProcessSDLEvent(const SDL_Event& event)
{
    DekiInput::InputEvent inputEvent;
    inputEvent.timestamp = static_cast<uint32_t>(SDL_GetTicks());

    if (event.type == SDL_EVENT_QUIT)
    {
        m_QuitFlag = true;
        inputEvent.type = DekiInput::InputEventType::AppQuit;
        NotifyCallbacks(inputEvent);
        return;
    }

    switch (event.type)
    {
        case SDL_EVENT_MOUSE_MOTION:
        {
            float ex = event.motion.x, ey = event.motion.y;
            WindowToFramebuffer(event.motion.windowID, ex, ey);
            inputEvent.type = DekiInput::InputEventType::MouseMove;
            inputEvent.x = (int32_t)ex;
            inputEvent.y = (int32_t)ey;
            NotifyCallbacks(inputEvent);
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            float ex = event.button.x, ey = event.button.y;
            WindowToFramebuffer(event.button.windowID, ex, ey);
            inputEvent.type = DekiInput::InputEventType::MouseButtonDown;
            inputEvent.x = (int32_t)ex;
            inputEvent.y = (int32_t)ey;
            inputEvent.pressed = true;
            NotifyCallbacks(inputEvent);
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            float ex = event.button.x, ey = event.button.y;
            WindowToFramebuffer(event.button.windowID, ex, ey);
            inputEvent.type = DekiInput::InputEventType::MouseButtonUp;
            inputEvent.x = (int32_t)ex;
            inputEvent.y = (int32_t)ey;
            inputEvent.pressed = false;
            NotifyCallbacks(inputEvent);
            break;
        }

        case SDL_EVENT_KEY_DOWN:
        {
            uint32_t genericKey = ConvertSDLKeyToGeneric(event.key.key);
            m_KeyStates[genericKey] = true;

            inputEvent.type = DekiInput::InputEventType::KeyDown;
            inputEvent.key = genericKey;
            inputEvent.pressed = true;
            NotifyCallbacks(inputEvent);
        }
        break;

        case SDL_EVENT_KEY_UP:
        {
            uint32_t genericKey = ConvertSDLKeyToGeneric(event.key.key);
            m_KeyStates[genericKey] = false;

            inputEvent.type = DekiInput::InputEventType::KeyUp;
            inputEvent.key = genericKey;
            inputEvent.pressed = false;
            NotifyCallbacks(inputEvent);
        }
        break;
    }
}

void SDL3Input::NotifyCallbacks(const DekiInput::InputEvent& event)
{
    for (const auto& callback : m_EventCallbacks)
    {
        callback(event);
    }
}

uint32_t SDL3Input::ConvertSDLKeyToGeneric(SDL_Keycode sdlKey)
{
    namespace Keys = DekiInput::Keys;

    switch (sdlKey)
    {
        case SDLK_RETURN: return Keys::Enter;
        case SDLK_ESCAPE: return Keys::Esc;
        case SDLK_BACKSPACE: return Keys::Backspace;
        case SDLK_TAB: return Keys::Tab;
        case SDLK_DELETE: return Keys::Delete;
        case SDLK_UP: return Keys::Up;
        case SDLK_DOWN: return Keys::Down;
        case SDLK_LEFT: return Keys::Left;
        case SDLK_RIGHT: return Keys::Right;
        default:
            if (sdlKey >= 32 && sdlKey <= 126)
            {
                return sdlKey;
            }
            return 0;
    }
}

void SDL3Input::RegisterEventCallback(const DekiInput::InputEventCallback& callback)
{
    m_EventCallbacks.push_back(callback);
}

bool SDL3Input::IsInitialized() const
{
    return initialized;
}

bool SDL3Input::GetPointerPosition(int32_t* x, int32_t* y) const
{
    if (x)
    {
        *x = m_MouseX;
    }
    if (y)
    {
        *y = m_MouseY;
    }
    return true;
}

bool SDL3Input::IsKeyPressed(uint32_t key) const
{
    auto it = m_KeyStates.find(key);
    if (it != m_KeyStates.end())
    {
        return it->second;
    }
    return false;
}

bool SDL3Input::CheckForQuit() const
{
    return m_QuitFlag;
}

}  // namespace DekiSdl3
