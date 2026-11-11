#pragma once

#include <deki/SetupComponent.h>
#include <deki/reflection/Property.h>
#include "SDL3Package.h"

namespace DekiSdl3
{

/// Starts SDL3 keyboard, mouse and gamepad input. Add it to the boot scene; it
/// runs as part of the boot sequence.
DEKI_CATEGORY("SDL3")
DEKI_DESCRIPTION("Feeds desktop keyboard, mouse and gamepad input to the engine.")
class DEKI_SDL3_API SDL3InputSetup : public Deki::SetupComponent
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("Enable keyboard input")
    bool enableKeyboard = true;

    DEKI_EXPORT
    DEKI_TOOLTIP("Enable mouse/pointer input")
    bool enableMouse = true;

    DEKI_EXPORT
    DEKI_TOOLTIP("Read gamepads as keys, the same keys a handheld's buttons send: D-pad and left stick are the "
                 "arrows, A and Start Enter, B Esc, X Space, Y Tab, Back Backspace, LB and RB Q and E, LT and RT "
                 "Z and C. Its events come from the input source \"Gamepad\".")
    bool enableGamepad = true;

    void Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "SDL3 Input"; }
};

}  // namespace DekiSdl3
