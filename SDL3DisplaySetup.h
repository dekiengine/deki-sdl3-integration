#pragma once

#include <cstdint>
#include <deki/SetupComponent.h>
#include <deki/reflection/Property.h>
#include "SDL3Package.h"

namespace DekiSdl3
{

/// Opens the desktop window that stands in for a device's screen. The screen
/// is the target platform's: the simulator build compiles in its size and
/// colour format as DEKI_SCREEN_WIDTH/HEIGHT/COLOR_FORMAT, and the game renders
/// at exactly that, as on the device. The window shows it at a whole-number
/// scale. Runs as part of the boot sequence.
DEKI_CATEGORY("SDL3")
DEKI_DESCRIPTION("Opens the desktop window the game renders into.")
class DEKI_SDL3_API SDL3DisplaySetup : public Deki::SetupComponent
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("How many window pixels show one screen pixel. The screen's size is the target platform's.")
    DEKI_RANGE(1, 8)
    int32_t windowScale = 2;

    void Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "SDL3 Display"; }
};

}  // namespace DekiSdl3
