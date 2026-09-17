#include "SDL3InputSetup.h"
#include <deki/LogSystem.h>

#if !defined(DEKI_EDITOR) && defined(DEKI_PACKAGE_SDL3)
#include "SDL3Input.h"
#include "DekiInput.h"  // from deki-input
#include "DekiInputInit.h"  // DekiInput_InitSystem (from deki-input)
#include <deki/Engine.h>
#include <deki/providers/IInputSystem.h>
#endif

namespace DekiSdl3
{

#if !defined(DEKI_EDITOR) && defined(DEKI_PACKAGE_SDL3)


void SDL3InputSetup::Setup(SetupCallback onComplete)
{
    DEKI_LOG_INFO("SDL3InputSetup: Initializing SDL3 input (keyboard=%d, mouse=%d)",
                  enableKeyboard, enableMouse);

    auto input = std::make_unique<SDL3Input>();
    if (input->Initialize())
    {
        // deki-input 0.16.0 put its types in a namespace that shares its name
        // with the class, so from outside the package the class is
        // DekiInput::DekiInput and a bare DekiInput:: finds the namespace,
        // where SetInput is not a member. The alias says which one is meant
        // once, instead of repeating the doubled name at each call.
        using DekiInputApi = DekiInput::DekiInput;
        DekiInputApi::SetInput(std::move(input), "SDL3");

        // Ensure the input dispatch system (deki-input) is created, initialized, and
        // registered with the engine. On editor/firmware builds the generated
        // deki_init_package_systems() already does this; the static desktop sim links the
        // engine's empty stub for that symbol, so do it here. Idempotent if already done.
        // Global, not DekiInput::, because the editor's generated glue declares
        // it that way and the package keeps it outside its namespace to match.
        DekiInput_InitSystem();

        DEKI_LOG_INFO("SDL3InputSetup: Input initialized successfully");
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
    // Editor/ESP32: SDL3 input not applicable
    onComplete(true);
}

#endif

}  // namespace DekiSdl3
