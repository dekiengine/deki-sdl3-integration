#pragma once

#include <deki/SetupComponent.h>
#include <deki/reflection/Property.h>
#include "SDL3Package.h"

namespace DekiSdl3
{

/// Plays deki-audio's sound through the computer's speakers, or the browser's.
/// Add it to the boot scene. Without deki-audio in the project it does
/// nothing. A game without sound still starts.
DEKI_CATEGORY("SDL3")
DEKI_DISPLAY_NAME("SDL3 Audio")
DEKI_DESCRIPTION("Plays the game's audio through the computer's speakers.")
class DEKI_SDL3_API SDL3AudioSetup : public Deki::SetupComponent
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("Samples per second the game mixes at. 48000 suits most computers; SDL converts to the device's "
                 "own rate when it differs.")
    DEKI_UNIT(Frequency)
    DEKI_RANGE(8000, 96000)
    int32_t sampleRate = 48000;

    DEKI_EXPORT
    DEKI_TOOLTIP("1 for mono, 2 for stereo.")
    DEKI_RANGE(1, 2)
    int32_t channels = 2;

    DEKI_EXPORT
    DEKI_TOOLTIP("The master volume at startup, 0 to 100.")
    DEKI_RANGE(0, 100)
    int32_t initialVolume = 50;

    void Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "SDL3 Audio"; }
};

}  // namespace DekiSdl3
