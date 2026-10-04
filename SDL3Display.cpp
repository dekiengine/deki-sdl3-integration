#include "SDL3Display.h"
#include <deki/providers/Memory.h>

#include <cstring>
#include <memory>

#include <deki/Engine.h>
#include <deki/Time.h>
#include "SDL3TimeProvider.h"

namespace DekiSdl3
{

namespace
{
// SDL3 supplies the engine's time source (SDL_GetTicks). A static initializer
// registers it, so Deki::Time has a provider before main() and
// Deki::Engine::Initialize(). It must live here, not in SDL3Package.cpp: that
// file is the package's DLL entry and is left out of the static simulator link.
struct SDL3TimeInit
{
    SDL3TimeInit() { Deki::Time::SetTimeProvider(std::make_unique<SDL3TimeProvider>()); }
};
static SDL3TimeInit s_Sdl3TimeInit;
}  // namespace

SDL3Display::SDL3Display()
    : window(nullptr),
      renderer(nullptr),
      m_GameTexture(nullptr),
      m_UiOverlayTexture(nullptr),
      m_DisplayWidth(0),
      m_DisplayHeight(0),
      initialized(false),
      m_LastFbWidth(0),
      m_LastFbHeight(0)
{
}

SDL3Display::~SDL3Display()
{
    Shutdown();
}

bool SDL3Display::Initialize(int32_t width, int32_t height)
{
    if (initialized)
    {
        return true;
    }

    m_DisplayWidth = width;
    m_DisplayHeight = height;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        DEKI_LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    // The window shows the emulated screen at a whole-number scale.
    window = SDL_CreateWindow("Deki", width * m_WindowScale, height * m_WindowScale, 0);
    if (window == nullptr)
    {
        DEKI_LOG_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    // Must use the OpenGL backend. SDL's default on Windows is Direct3D11, whose
    // DXGI present can block forever a few seconds in on Intel integrated GPUs
    // (the main thread hangs in dxgi.dll!Present). SDL3 with OpenGL works on the
    // same machines.
    renderer = SDL_CreateRenderer(window, "opengl");
    if (renderer == nullptr)
    {
        DEKI_LOG_ERROR("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // Draw at the screen's own size and let SDL scale it up to the window by a
    // whole number (nearest neighbour, pixelated like a device panel).
    SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

    // No vsync: a vsync-locked present blocks for seconds when the window stops
    // getting vblanks (hidden or in the background). The engine's own frame
    // limiter (Deki::Time::Delay and the target FPS) paces frames.
    SDL_SetRenderVSync(renderer, 0);

    // The UI overlay texture is created on demand.
    m_UiOverlayTexture = nullptr;

    initialized = true;
    DEKI_LOG_INTERNAL("SDL3 display initialized: %dx%d screen, window x%d", width, height, (int)m_WindowScale);

    return true;
}

void SDL3Display::Shutdown()
{
    if (!initialized)
    {
        return;
    }

    // UI overlay cleanup is handled separately

    if (m_UiOverlayTexture)
    {
        SDL_DestroyTexture(m_UiOverlayTexture);
        m_UiOverlayTexture = nullptr;
    }

    if (m_GameTexture)
    {
        SDL_DestroyTexture(m_GameTexture);
        m_GameTexture = nullptr;
    }

    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
    initialized = false;
}

bool SDL3Display::EnsureGameTexture(int width, int height, Deki::ColorFormat format)
{
    if (m_GameTexture && width == m_LastFbWidth && height == m_LastFbHeight && format == m_LastFbFormat)
    {
        return false;
    }

    if (m_GameTexture)
    {
        SDL_DestroyTexture(m_GameTexture);
        m_GameTexture = nullptr;
    }

    SDL_PixelFormat sdlFormat;
    switch (format)
    {
        case Deki::ColorFormat::RGB565: sdlFormat = SDL_PIXELFORMAT_RGB565; break;
        case Deki::ColorFormat::RGB888: sdlFormat = SDL_PIXELFORMAT_XRGB8888; break;
        case Deki::ColorFormat::ARGB8888: sdlFormat = SDL_PIXELFORMAT_ARGB8888; break;
        default: sdlFormat = SDL_PIXELFORMAT_RGB565; break;
    }

    m_GameTexture = SDL_CreateTexture(renderer, sdlFormat, SDL_TEXTUREACCESS_STREAMING, width, height);
    // Nearest-neighbour sampling keeps the upscale to the window pixel-perfect;
    // SDL3's default is linear, which blurs sprites.
    if (m_GameTexture)
    {
        SDL_SetTextureScaleMode(m_GameTexture, SDL_SCALEMODE_NEAREST);
    }
    m_LastFbWidth = width;
    m_LastFbHeight = height;
    m_LastFbFormat = format;
    return true;
}

void SDL3Display::DrawWindow()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (m_GameTexture)
    {
        SDL_RenderTexture(renderer, m_GameTexture, nullptr, nullptr);
    }

    if (m_UiOverlayTexture)
    {
        SDL_RenderTexture(renderer, m_UiOverlayTexture, nullptr, nullptr);
    }

    SDL_RenderPresent(renderer);
}

void SDL3Display::Present(const uint8_t* framebuffer, int width, int height, Deki::ColorFormat format)
{
    if (!initialized || !renderer)
    {
        return;
    }

    if (framebuffer)
    {
        EnsureGameTexture(width, height, format);

        if (m_GameTexture)
        {
            void* pixels;
            int pitch;
            if (SDL_LockTexture(m_GameTexture, nullptr, &pixels, &pitch))
            {
                int bytesPerPixel;
                switch (format)
                {
                    case Deki::ColorFormat::RGB565: bytesPerPixel = 2; break;
                    case Deki::ColorFormat::RGB888: bytesPerPixel = 3; break;
                    case Deki::ColorFormat::ARGB8888: bytesPerPixel = 4; break;
                    default: bytesPerPixel = 2; break;
                }
                memcpy(pixels, framebuffer, width * height * bytesPerPixel);
                SDL_UnlockTexture(m_GameTexture);
            }
        }
    }

    DrawWindow();
}

void SDL3Display::PresentRegions(const uint8_t* framebuffer, int width, int height, Deki::ColorFormat format,
                                 const Deki::Rect* rects, int32_t count)
{
    if (!initialized || !renderer)
    {
        return;
    }

    // Per-rectangle uploads for the 2-byte RGB565 layout (the texture's rows
    // match the framebuffer's). The other formats keep the whole-frame path;
    // so does a texture that was just recreated, whose contents are undefined.
    if (!framebuffer || format != Deki::ColorFormat::RGB565 || EnsureGameTexture(width, height, format) ||
        !m_GameTexture)
    {
        Present(framebuffer, width, height, format);
        return;
    }

    for (int32_t i = 0; i < count; ++i)
    {
        const Deki::Rect& r = rects[i];
        if (r.Empty())
        {
            continue;
        }
        SDL_Rect sr{ r.left, r.top, r.Width(), r.Height() };
        void* pixels;
        int pitch;
        if (!SDL_LockTexture(m_GameTexture, &sr, &pixels, &pitch))
        {
            continue;
        }
        const size_t rowBytes = static_cast<size_t>(r.Width()) * 2;
        for (int32_t y = 0; y < r.Height(); ++y)
        {
            memcpy(static_cast<uint8_t*>(pixels) + static_cast<size_t>(y) * pitch,
                   framebuffer + (static_cast<size_t>(r.top + y) * width + r.left) * 2, rowBytes);
        }
        SDL_UnlockTexture(m_GameTexture);
    }

    // The window is redrawn from the texture every frame, changed or not.
    DrawWindow();
}

void SDL3Display::GetDisplaySize(int32_t* width, int32_t* height) const
{
    if (width)
    {
        *width = m_DisplayWidth;
    }
    if (height)
    {
        *height = m_DisplayHeight;
    }
}

bool SDL3Display::IsInitialized() const
{
    return initialized;
}

void SDL3Display::RequestFullRefresh()
{
    // Nothing to do: the window is redrawn in full every frame.
}

bool SDL3Display::ProcessEvents()
{
    // SDL3Input handles events through DekiInput, and DekiInput::ShouldExit()
    // detects quitting, so this only keeps the program running.
    return true;
}

void* SDL3Display::CreateUIOverlay(int32_t width, int32_t height)
{
    if (!initialized || !renderer)
    {
        return nullptr;
    }

    SDL_Texture* overlay =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);

    if (overlay == nullptr)
    {
        DEKI_LOG_WARNING("SDL_CreateTexture for UI overlay failed: %s", SDL_GetError());
        return nullptr;
    }

    SDL_SetTextureBlendMode(overlay, SDL_BLENDMODE_BLEND);

    SDL_SetRenderTarget(renderer, overlay);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);  // Transparent black
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, nullptr);  // Back to the window

    return overlay;
}

bool SDL3Display::UpdateUIOverlay(void* overlay, int32_t x, int32_t y, int32_t width, int32_t height,
                                  const uint32_t* pixels)
{
    if (!overlay || !pixels)
    {
        return false;
    }

    SDL_Texture* texture = (SDL_Texture*)overlay;
    SDL_Rect rect = { x, y, width, height };

    if (!SDL_UpdateTexture(texture, &rect, pixels, width * 4))
    {
        DEKI_LOG_WARNING("SDL_UpdateTexture failed: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool SDL3Display::UpdateUIOverlayRGB565A8(void* overlay, int32_t x, int32_t y, int32_t width, int32_t height,
                                          const uint8_t* rgb565a8Pixels)
{
    if (!overlay || !rgb565a8Pixels)
    {
        return false;
    }

    // RGB565A8 ([RGB565 low, RGB565 high, alpha] per pixel) to ARGB8888 for SDL.
    int pixelCount = width * height;
    // A whole-screen conversion scratch buffer.
    uint32_t* argb8888Buffer =
        Deki::Memory::AllocateArray<uint32_t>(static_cast<size_t>(pixelCount), Deki::Memory::External);
    if (!argb8888Buffer)
    {
        return false;
    }

    for (int i = 0; i < pixelCount; i++)
    {
        int idx = i * 3;
        uint16_t rgb565 = rgb565a8Pixels[idx] | (rgb565a8Pixels[idx + 1] << 8);
        uint8_t alpha = rgb565a8Pixels[idx + 2];

        uint8_t r = ((rgb565 >> 11) & 0x1F) << 3;  // 5 bits -> 8 bits
        uint8_t g = ((rgb565 >> 5) & 0x3F) << 2;   // 6 bits -> 8 bits
        uint8_t b = (rgb565 & 0x1F) << 3;          // 5 bits -> 8 bits

        // Fill the low bits so white stays full white.
        r |= r >> 5;
        g |= g >> 6;
        b |= b >> 5;

        argb8888Buffer[i] = (alpha << 24) | (r << 16) | (g << 8) | b;
    }

    SDL_Texture* texture = (SDL_Texture*)overlay;
    SDL_Rect rect = { x, y, width, height };

    bool ok = SDL_UpdateTexture(texture, &rect, argb8888Buffer, width * 4);
    Deki::Memory::Free(argb8888Buffer);

    if (!ok)
    {
        DEKI_LOG_WARNING("SDL_UpdateTexture (RGB565A8) failed: %s", SDL_GetError());
        return false;
    }

    return true;
}

void SDL3Display::DestroyUIOverlay(void* overlay)
{
    if (overlay)
    {
        SDL_Texture* texture = (SDL_Texture*)overlay;

        if (texture == m_UiOverlayTexture)
        {
            m_UiOverlayTexture = nullptr;
        }

        SDL_DestroyTexture(texture);
    }
}

void SDL3Display::SetActiveUIOverlay(void* overlay)
{
    m_UiOverlayTexture = (SDL_Texture*)overlay;
}

void SDL3Display::ClearActiveUIOverlay()
{
    if (!m_UiOverlayTexture)
    {
        return;
    }

    float w, h;
    SDL_GetTextureSize(m_UiOverlayTexture, &w, &h);

    int iw = (int)w, ih = (int)h;
    size_t bufferSize = iw * ih * sizeof(uint32_t);
    // The engine's allocator returns zeroed memory, which is the transparent
    // buffer needed here.
    uint32_t* clearBuffer = (uint32_t*)Deki::Memory::Allocate(bufferSize, Deki::Memory::External);
    if (clearBuffer)
    {
        SDL_UpdateTexture(m_UiOverlayTexture, nullptr, clearBuffer, iw * sizeof(uint32_t));
        Deki::Memory::Free(clearBuffer);
    }
}

}  // namespace DekiSdl3
