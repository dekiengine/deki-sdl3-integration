#pragma once

#include <deki/SetupComponent.h>
#include <deki/reflection/Property.h>
#include "SDL3Package.h"

/**
 * @brief Component to configure and initialize SDL3 keyboard/mouse input
 *
 * Add this component to your boot scene to enable SDL3 input.
 *
 * Inherits from SetupComponent to participate in boot sequence.
 */
DEKI_CATEGORY("SDL3")
DEKI_DESCRIPTION("Feeds desktop keyboard and mouse input to the engine.")
class DEKI_SDL3_API SDL3InputSetup : public Deki::SetupComponent
{
public:

    DEKI_EXPORT
    DEKI_TOOLTIP("Enable keyboard input")
    bool enableKeyboard = true;

    DEKI_EXPORT
    DEKI_TOOLTIP("Enable mouse/pointer input")
    bool enableMouse = true;

    void Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "SDL3 Input"; }
};

// Generated property metadata
