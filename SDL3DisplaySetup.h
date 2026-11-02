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

    DEKI_EXPORT
    DEKI_TOOLTIP("Show frames through OpenGL where the platform has it. The frame is then built on the GPU, and "
                 "3D draws straight into it instead of being read back. The frame buffer becomes RGB565A8, "
                 "whatever the platform's screen format, so 2D can be composed in layers. Off, or without "
                 "OpenGL, SDL's GPU API shows the frame buffer as it is.")
    bool presentThroughOpenGL = true;

    DEKI_EXPORT
    DEKI_TOOLTIP("Presenting through OpenGL, draw the 2D on the GPU too: sprites, text and shapes go straight into "
                 "the frame and the CPU draws no pixels. Off, the 2D is drawn on the CPU and the frame buffer "
                 "uploaded, as on a device. The frame buffer then holds only what code writes into it directly, "
                 "so a screenshot of it misses the 2D.")
    bool draw2DOnGpu = true;

    void Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "SDL3 Display"; }
};

}  // namespace DekiSdl3
