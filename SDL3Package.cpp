// Package entry point for the deki-sdl3 DLL. Exports the standard Deki plugin
// interface, so the editor can load the DLL and find its components.
//
// Display and input are set up by their SetupComponents; the SDL3 time
// provider is registered in SDL3Display.cpp. The desktop main() and the
// memory and filesystem layers live in the deki-desktop-integration package.

#include "SDL3Package.h"
#include <deki/interop/Plugin.h>
#include <deki/Time.h>
#include <deki/providers/ITimeProvider.h>
#include <deki/reflection/ComponentRegistry.h>
#include <deki/reflection/ComponentFactory.h>

extern void DekiSDL3RegisterComponents();
extern int DekiSDL3GetAutoComponentCount();
extern const Deki::ComponentMeta* DekiSDL3GetAutoComponentMeta(int index);

namespace DekiSdl3
{

#ifdef DEKI_EDITOR

static bool s_SDL3Registered = false;

// The exports below are C symbols at global scope; the package's own
// registration helpers and statics live in its namespace.
using namespace DekiSdl3;

extern "C"
{
    // Registers the package's components once. Returns how many there are.
    DEKI_SDL3_API int DekiSDL3EnsureRegistered(void)
    {
        if (s_SDL3Registered)
        {
            return ::DekiSDL3GetAutoComponentCount();
        }
        s_SDL3Registered = true;

        // Generated: registers every SDL3 component with ComponentRegistry and ComponentFactory.
        ::DekiSDL3RegisterComponents();

        return ::DekiSDL3GetAutoComponentCount();
    }

    // =============================================================================
    // Plugin metadata
    // =============================================================================

    DEKI_PLUGIN_API const char* DekiPluginGetName(void)
    {
        return "Deki SDL3 Package";
    }

    DEKI_PLUGIN_API const char* DekiPluginGetVersion(void)
    {
#ifdef DEKI_PACKAGE_VERSION
        return DEKI_PACKAGE_VERSION;
#else
        return "0.0.0-dev";
#endif
    }

    DEKI_PLUGIN_API int DekiPluginInit(void)
    {
        return 0;
    }

    DEKI_PLUGIN_API void DekiPluginShutdown(void)
    {
        s_SDL3Registered = false;
        Deki::Time::SetTimeProvider(nullptr);
    }

    DEKI_PLUGIN_API int DekiPluginGetComponentCount(void)
    {
        return ::DekiSDL3GetAutoComponentCount();
    }

    DEKI_PLUGIN_API const Deki::ComponentMeta* DekiPluginGetComponentMeta(int index)
    {
        return ::DekiSDL3GetAutoComponentMeta(index);
    }

    DEKI_PLUGIN_API void DekiPluginRegisterComponents(void)
    {
        DekiSDL3EnsureRegistered();
    }

    // =============================================================================
    // Package-specific API, with names that do not clash when DLLs link each other
    // =============================================================================

    DEKI_SDL3_API const char* DekiSDL3GetName(void)
    {
        return "SDL3";
    }

}  // extern "C"

#else  // !DEKI_EDITOR - Runtime registration

// Components are registered by the generated ::DekiSDL3RegisterComponents(),
// called from DekiRegisterProjectPackages(). Display and input run as boot
// SetupComponents, and a static initializer in SDL3Display.cpp registers the
// SDL3 time provider.

#endif  // DEKI_EDITOR
}  // namespace DekiSdl3
