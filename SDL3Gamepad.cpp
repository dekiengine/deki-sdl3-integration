#include "SDL3Gamepad.h"
#include "Keys.h"  // from deki-input

#include <algorithm>

namespace DekiSdl3
{

namespace
{
// A stick or trigger counts once it is past this, of SDL's 32767.
constexpr Sint16 kStickThreshold = 16000;
constexpr Sint16 kTriggerThreshold = 16000;

// The keys a pad sends, in the order they are checked.
constexpr uint32_t kKeys[] = { DekiInput::Keys::Up,        DekiInput::Keys::Down,  DekiInput::Keys::Left,
                               DekiInput::Keys::Right,     DekiInput::Keys::Enter, DekiInput::Keys::Esc,
                               DekiInput::Keys::Space,     DekiInput::Keys::Tab,   DekiInput::Keys::Backspace,
                               DekiInput::Keys::ForCharacter('q'), DekiInput::Keys::ForCharacter('e'),
                               DekiInput::Keys::ForCharacter('z'), DekiInput::Keys::ForCharacter('c') };

bool PadSends(SDL_Gamepad* pad, uint32_t key)
{
    auto button = [pad](SDL_GamepadButton b) { return SDL_GetGamepadButton(pad, b); };
    auto axis = [pad](SDL_GamepadAxis a) { return SDL_GetGamepadAxis(pad, a); };
    switch (key)
    {
        case DekiInput::Keys::Up: return button(SDL_GAMEPAD_BUTTON_DPAD_UP) || axis(SDL_GAMEPAD_AXIS_LEFTY) < -kStickThreshold;
        case DekiInput::Keys::Down: return button(SDL_GAMEPAD_BUTTON_DPAD_DOWN) || axis(SDL_GAMEPAD_AXIS_LEFTY) > kStickThreshold;
        case DekiInput::Keys::Left: return button(SDL_GAMEPAD_BUTTON_DPAD_LEFT) || axis(SDL_GAMEPAD_AXIS_LEFTX) < -kStickThreshold;
        case DekiInput::Keys::Right: return button(SDL_GAMEPAD_BUTTON_DPAD_RIGHT) || axis(SDL_GAMEPAD_AXIS_LEFTX) > kStickThreshold;
        case DekiInput::Keys::Enter: return button(SDL_GAMEPAD_BUTTON_SOUTH) || button(SDL_GAMEPAD_BUTTON_START);
        case DekiInput::Keys::Esc: return button(SDL_GAMEPAD_BUTTON_EAST);
        case DekiInput::Keys::Space: return button(SDL_GAMEPAD_BUTTON_WEST);
        case DekiInput::Keys::Tab: return button(SDL_GAMEPAD_BUTTON_NORTH);
        case DekiInput::Keys::Backspace: return button(SDL_GAMEPAD_BUTTON_BACK);
        case DekiInput::Keys::ForCharacter('q'): return button(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
        case DekiInput::Keys::ForCharacter('e'): return button(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
        case DekiInput::Keys::ForCharacter('z'): return axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > kTriggerThreshold;
        case DekiInput::Keys::ForCharacter('c'): return axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > kTriggerThreshold;
        default: return false;
    }
}
}  // namespace

SDL3Gamepad::~SDL3Gamepad()
{
    Shutdown();
}

bool SDL3Gamepad::Initialize()
{
    if (m_Initialized)
    {
        return true;
    }
    m_Initialized = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    return m_Initialized;
}

void SDL3Gamepad::Shutdown()
{
    if (!m_Initialized)
    {
        return;
    }
    for (SDL_Gamepad* pad : m_Pads)
    {
        SDL_CloseGamepad(pad);
    }
    m_Pads.clear();
    m_Down.clear();
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    m_Initialized = false;
}

void SDL3Gamepad::RegisterEventCallback(const DekiInput::InputEventCallback& callback)
{
    m_Callbacks.push_back(callback);
}

bool SDL3Gamepad::GetPointerPosition(int32_t*, int32_t*) const
{
    return false;
}

bool SDL3Gamepad::IsKeyPressed(uint32_t key) const
{
    return m_Down.count(key) != 0;
}

void SDL3Gamepad::RefreshPads()
{
    // Pads unplugged since the last update go; new ones are opened.
    m_Pads.erase(std::remove_if(m_Pads.begin(), m_Pads.end(),
                                [](SDL_Gamepad* pad)
                                {
                                    if (SDL_GamepadConnected(pad))
                                    {
                                        return false;
                                    }
                                    SDL_CloseGamepad(pad);
                                    return true;
                                }),
                 m_Pads.end());
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    for (int i = 0; ids && i < count; ++i)
    {
        const bool open = std::any_of(m_Pads.begin(), m_Pads.end(),
                                      [&](SDL_Gamepad* pad) { return SDL_GetGamepadID(pad) == ids[i]; });
        if (!open)
        {
            if (SDL_Gamepad* pad = SDL_OpenGamepad(ids[i]))
            {
                m_Pads.push_back(pad);
            }
        }
    }
    SDL_free(ids);
}

bool SDL3Gamepad::KeyIsDown(uint32_t key) const
{
    return std::any_of(m_Pads.begin(), m_Pads.end(), [key](SDL_Gamepad* pad) { return PadSends(pad, key); });
}

void SDL3Gamepad::Emit(DekiInput::InputEventType type, uint32_t key)
{
    DekiInput::InputEvent event{};
    event.type = type;
    event.key = key;
    event.pressed = type == DekiInput::InputEventType::KeyDown;
    event.timestamp = static_cast<uint32_t>(SDL_GetTicks());
    for (const auto& callback : m_Callbacks)
    {
        callback(event);
    }
}

void SDL3Gamepad::Update()
{
    if (!m_Initialized)
    {
        return;
    }
    SDL_UpdateGamepads();
    RefreshPads();
    for (uint32_t key : kKeys)
    {
        const bool down = KeyIsDown(key);
        const bool was = m_Down.count(key) != 0;
        if (down && !was)
        {
            m_Down.insert(key);
            Emit(DekiInput::InputEventType::KeyDown, key);
        }
        else if (!down && was)
        {
            m_Down.erase(key);
            Emit(DekiInput::InputEventType::KeyUp, key);
        }
    }
}

}  // namespace DekiSdl3
