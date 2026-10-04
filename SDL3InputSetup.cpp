#include "SDL3InputSetup.h"
#include <deki/LogSystem.h>

#if !defined(DEKI_EDITOR) && defined(DEKI_PACKAGE_SDL3)
#include "SDL3Input.h"
#include "DekiInput.h"      // from deki-input
#include "DekiInputInit.h"  // DekiInputInitSystem (from deki-input)
#include <deki/Engine.h>
#include <deki/providers/IInputSystem.h>
#endif

namespace DekiSdl3
{

#if !defined(DEKI_EDITOR) && defined(DEKI_PACKAGE_SDL3)

void SDL3InputSetup::Setup(SetupCallback onComplete)
{
    DEKI_LOG_INFO("SDL3InputSetup: Initializing SDL3 input (keyboard=%d, mouse=%d)", enableKeyboard, enableMouse);

    auto input = std::make_unique<SDL3Input>();
    if (input->Initialize())
    {
        // deki-input's namespace and class share the name DekiInput, so a bare
        // DekiInput:: finds the namespace, where SetInput is not a member. The
        // alias names the class once.
        using DekiInputApi = DekiInput::DekiInput;
        DekiInputApi::SetInput(std::move(input), "SDL3");

        // Start deki-input's dispatch system and register it with the engine.
        // Editor and firmware builds do this in the generated
        // DekiInitPackageSystems(), but the static desktop simulator links the
        // engine's empty stub for it, so it happens here too. Safe to repeat.
        // Global, not DekiInput::, because the editor's generated glue
        // declares it that way.
        DekiInputInitSystem();

        onComplete(true);
    }
    else
    {
        DEKI_LOG_ERROR("SDL3InputSetup: Failed to initialize SDL3 input");
        onComplete(false);
    }
}

#else

void SDL3InputSetup::Setup(SetupCallback onComplete)
{
    // The editor and device builds take input elsewhere.
    onComplete(true);
}

#endif

}  // namespace DekiSdl3
