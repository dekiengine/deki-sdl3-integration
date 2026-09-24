#pragma once

#include <cstdint>
#include <deki/SetupComponent.h>
#include <deki/reflection/Property.h>
#include "SDL3Package.h"

namespace DekiSdl3
{

/**
 * @brief Opens the desktop window that stands in for a device's screen.
 *
 * The screen it emulates is the target platform's (screenWidth x screenHeight,
 * compiled in as DEKI_SCREEN_WIDTH/HEIGHT by the simulator build); the game
 * renders at exactly that size, as it would on the device. The window shows it
 * at a whole-number scale.
 *
 * Inherits from SetupComponent to participate in boot sequence.
 */
DEKI_CATEGORY("SDL3")
DEKI_DESCRIPTION("Opens the desktop window the game renders into.")
DEKI_FORMER_NAME("SDL3DisplaySetup")
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

// Generated property metadata

}  // namespace DekiSdl3
