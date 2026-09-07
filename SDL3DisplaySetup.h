#pragma once

#include <cstdint>
#include <deki/SetupComponent.h>
#include <deki/reflection/Property.h>
#include "SDL3Package.h"

/**
 * @brief Component to configure and initialize an SDL3 display window
 *
 * Add this component to your boot scene to set up an SDL3 window.
 * Configure the window dimensions and scale in the Inspector.
 *
 * Inherits from SetupComponent to participate in boot sequence.
 */
DEKI_CATEGORY("SDL3")
DEKI_DESCRIPTION("Opens the desktop window the game renders into.")
class DEKI_SDL3_API SDL3DisplaySetup : public Deki::SetupComponent
{
public:

    DEKI_EXPORT
    DEKI_TOOLTIP("Window width in pixels")
    DEKI_RANGE(64, 3840)
    int32_t windowWidth = 320;

    DEKI_EXPORT
    DEKI_TOOLTIP("Window height in pixels")
    DEKI_RANGE(64, 2160)
    int32_t windowHeight = 240;

    DEKI_EXPORT
    DEKI_TOOLTIP("Window scale multiplier")
    DEKI_RANGE(1, 8)
    int32_t windowScale = 2;

    void Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "SDL3 Display"; }
};

// Generated property metadata
