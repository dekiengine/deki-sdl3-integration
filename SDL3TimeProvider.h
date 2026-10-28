#pragma once

#include <SDL3/SDL.h>

#include <deki/providers/ITimeProvider.h>

namespace DekiSdl3
{

class SDL3TimeProvider : public Deki::ITimeProvider
{
public:
    uint32_t GetTicksMs() const override { return static_cast<uint32_t>(SDL_GetTicks()); }
    uint64_t GetTicksUs() const override { return SDL_GetTicksNS() / 1000u; }
    void DelayMs(uint32_t ms) const override { SDL_Delay(ms); }
};

}  // namespace DekiSdl3
