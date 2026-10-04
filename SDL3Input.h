#pragma once

#include <SDL3/SDL.h>

#include <unordered_map>
#include <vector>

#include "IDekiInput.h"  // from deki-input

namespace DekiSdl3
{

/// Keyboard and mouse input through SDL3.
class SDL3Input : public DekiInput::IDekiInput
{
private:
    bool initialized;
    std::vector<DekiInput::InputEventCallback> m_EventCallbacks;
    bool m_QuitFlag;  // Set when SDL reports a quit event

    std::unordered_map<uint32_t, bool> m_KeyStates;

    int32_t m_MouseX, m_MouseY;
    bool m_MousePressed;

    void ProcessSDLEvent(const SDL_Event& event);
    void NotifyCallbacks(const DekiInput::InputEvent& event);
    uint32_t ConvertSDLKeyToGeneric(SDL_Keycode sdlKey);

public:
    SDL3Input();
    virtual ~SDL3Input();

    // DekiInput::IDekiInput interface
    bool Initialize() override;
    void Shutdown() override;
    void Update() override;
    void RegisterEventCallback(const DekiInput::InputEventCallback& callback) override;
    bool IsInitialized() const override;
    bool GetPointerPosition(int32_t* x, int32_t* y) const override;
    bool IsKeyPressed(uint32_t key) const override;

    /// True once SDL has reported a quit event (the window was closed).
    bool CheckForQuit() const;
};

}  // namespace DekiSdl3
