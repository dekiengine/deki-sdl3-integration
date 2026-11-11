#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <set>
#include <vector>

#include "IDekiInput.h"  // from deki-input

namespace DekiSdl3
{

/// Gamepads through SDL3, as keys: the same keys a handheld's buttons send,
/// so a game made for one plays with the other.
///
///   D-pad, left stick   Up, Down, Left, Right
///   A (south), Start    Enter
///   B (east)            Esc
///   X (west)            Space
///   Y (north)           Tab
///   Back                Backspace
///   LB, RB              Q, E
///   LT, RT              Z, C
///
/// Every connected pad counts, and one plugged in while the game runs is
/// picked up. Registered as the input source "Gamepad".
class SDL3Gamepad : public DekiInput::IDekiInput
{
public:
    static constexpr const char* kSourceName = "Gamepad";

    ~SDL3Gamepad() override;

    bool Initialize() override;
    void Shutdown() override;
    void Update() override;
    void RegisterEventCallback(const DekiInput::InputEventCallback& callback) override;
    bool IsInitialized() const override { return m_Initialized; }
    bool GetPointerPosition(int32_t* x, int32_t* y) const override;
    bool IsKeyPressed(uint32_t key) const override;

    /// How many pads are open.
    size_t PadCount() const { return m_Pads.size(); }

private:
    void RefreshPads();
    bool KeyIsDown(uint32_t key) const;
    void Emit(DekiInput::InputEventType type, uint32_t key);

    bool m_Initialized = false;
    std::vector<SDL_Gamepad*> m_Pads;
    std::set<uint32_t> m_Down;
    std::vector<DekiInput::InputEventCallback> m_Callbacks;
};

}  // namespace DekiSdl3
