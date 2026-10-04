/**
 * @file SDL3Package.cpp
 * @brief Package entry point for deki-sdl3 DLL
 *
 * This file exports the standard Deki plugin interface so the editor
 * can load deki-sdl3.dll and discover available SDL3 components.
 *
 * Display and input are set up by their SetupComponents; the SDL3 time provider
 * is registered in SDL3Display.cpp. The desktop program entry (main) and the
 * memory/filesystem HAL live in the deki-desktop-integration package.
 */

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

// Auto-generated registration helpers

// Track if already registered to avoid duplicates
static bool s_SDL3Registered = false;

// The exports below are C symbols at global scope; the package's own
// registration helpers and statics live in its namespace.
using namespace DekiSdl3;

extern "C"
{
    /**
     * @brief Ensure deki-sdl3 package is loaded and components are registered
     */
    DEKI_SDL3_API int DekiSDL3EnsureRegistered(void)
    {
        if (s_SDL3Registered)
        {
            return ::DekiSDL3GetAutoComponentCount();
        }
        s_SDL3Registered = true;

        // Auto-generated: registers all SDL3 components with ComponentRegistry + ComponentFactory
        ::DekiSDL3RegisterComponents();

        return ::DekiSDL3GetAutoComponentCount();
    }

    // =============================================================================
    // Plugin metadata (for dynamic loading compatibility)
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
    // Package-specific feature API (for linked DLL access without name conflicts)
    // =============================================================================

    DEKI_SDL3_API const char* DekiSDL3GetName(void)
    {
        return "SDL3";
    }

}  // extern "C"

#else  // !DEKI_EDITOR - Runtime registration

// Component registration happens via the auto-generated ::DekiSDL3RegisterComponents(),
// called from DekiRegisterProjectPackages(). Display/input run as boot SetupComponents,
// and the SDL3 time provider is registered by a static initializer in SDL3Display.cpp.

#endif  // DEKI_EDITOR
}  // namespace DekiSdl3
