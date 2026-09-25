#include "SDL3DisplaySetup.h"
#include <deki/LogSystem.h>

#if !defined(DEKI_EDITOR) && defined(DEKI_PACKAGE_SDL3)
#include "SDL3Display.h"
#include <deki/Engine.h>
#endif

namespace DekiSdl3
{

#if !defined(DEKI_EDITOR) && defined(DEKI_PACKAGE_SDL3)


// Package owns the SDL3Display lifetime now (engine-core just holds the pointer).
// File-scope unique_ptr keeps it alive for the program's lifetime.
static std::unique_ptr<SDL3Display> s_SDL3Display;

#if !defined(DEKI_SCREEN_WIDTH) || !defined(DEKI_SCREEN_HEIGHT) || !defined(DEKI_SCREEN_COLOR_FORMAT)
#error "The simulator emulates the target platform's screen: its build must define DEKI_SCREEN_WIDTH, DEKI_SCREEN_HEIGHT and DEKI_SCREEN_COLOR_FORMAT (the platform's screenWidth, screenHeight and colorFormat)."
#endif

void SDL3DisplaySetup::Setup(SetupCallback onComplete)
{
    DEKI_LOG_INFO("SDL3DisplaySetup: Creating SDL3 display (%dx%d screen, window x%d)",
                  (int)DEKI_SCREEN_WIDTH, (int)DEKI_SCREEN_HEIGHT, (int)windowScale);

    s_SDL3Display = std::make_unique<SDL3Display>();
    if (s_SDL3Display)
    {
        s_SDL3Display->SetWindowScale(windowScale);
        s_SDL3Display->SetColorFormat(DEKI_SCREEN_COLOR_FORMAT);
    }
    if (s_SDL3Display && s_SDL3Display->Initialize(DEKI_SCREEN_WIDTH, DEKI_SCREEN_HEIGHT))
    {
        Deki::Engine::GetInstance().SetDisplay(s_SDL3Display.get(), "SDL3");
        DEKI_LOG_INFO("SDL3DisplaySetup: Display initialized successfully");
        onComplete(true);
    }
    else
    {
        DEKI_LOG_ERROR("SDL3DisplaySetup: Failed to initialize SDL3 display");
        s_SDL3Display.reset();
        onComplete(false);
    }
}

#else

void SDL3DisplaySetup::Setup(SetupCallback onComplete)
{
    // Editor/ESP32: SDL3 display not applicable
    onComplete(true);
}

#endif

}  // namespace DekiSdl3
