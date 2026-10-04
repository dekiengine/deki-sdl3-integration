#pragma once

// Main header of the SDL3 package. The package provides display setup (an SDL3
// window and its rendering) and input setup (keyboard and mouse).

#ifdef _WIN32
#ifdef DEKI_SDL3_EXPORTS
#define DEKI_SDL3_API __declspec(dllexport)
#else
#define DEKI_SDL3_API __declspec(dllimport)
#endif
#else
#define DEKI_SDL3_API __attribute__((visibility("default")))
#endif
