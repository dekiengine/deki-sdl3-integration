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
// SDL3 supplies the engine's time source (SDL_GetTicks). Register it via a static
// initializer so Deki::Time has a provider before main()/Deki::Engine::Initialize().
// (This was previously in SDL3Package.cpp, which is the package's DLL/editor entry and
// is excluded from the static simulator link.)
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

    // Initialize SDL
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

    // Create SDL renderer. Force the OpenGL backend: SDL's default on Windows is
    // Direct3D11, whose DXGI present blocks indefinitely a few seconds in on this
    // Intel iGPU (the main thread hangs deep in dxgi.dll!Present). The editor runs
    // SDL3+OpenGL reliably on the same machine, so OpenGL is the safe backend.
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

    // No vsync. A vsync-locked present blocks indefinitely when the window stops
    // receiving vblanks (occluded / not foreground) — that was the ~5-8s "hang".
    // The engine's own frame limiter (Deki::Time::Delay / target FPS) paces frames.
    SDL_SetRenderVSync(renderer, 0);

    // Initialize UI overlay texture (will be created on demand)
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

    // Choose SDL pixel format based on GameEngine format
    SDL_PixelFormat sdlFormat;
    switch (format)
    {
        case Deki::ColorFormat::RGB565: sdlFormat = SDL_PIXELFORMAT_RGB565; break;
        case Deki::ColorFormat::RGB888: sdlFormat = SDL_PIXELFORMAT_XRGB8888; break;
        case Deki::ColorFormat::ARGB8888: sdlFormat = SDL_PIXELFORMAT_ARGB8888; break;
        default: sdlFormat = SDL_PIXELFORMAT_RGB565; break;
    }

    m_GameTexture = SDL_CreateTexture(renderer, sdlFormat, SDL_TEXTUREACCESS_STREAMING, width, height);
    // Force nearest-neighbor sampling so logical→window upscale stays
    // pixel-perfect (SDL3 default is linear, which would blur sprites).
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
    // Clear with black background first
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (m_GameTexture)
    {
        SDL_RenderTexture(renderer, m_GameTexture, nullptr, nullptr);
    }

    // Render UI overlay on top if active
    if (m_UiOverlayTexture)
    {
        SDL_RenderTexture(renderer, m_UiOverlayTexture, nullptr, nullptr);
    }

    // Present the frame
    SDL_RenderPresent(renderer);
}

void SDL3Display::Present(const uint8_t* framebuffer, int width, int height, Deki::ColorFormat format)
{
    if (!initialized || !renderer)
    {
        return;
    }

    // Render GameEngine framebuffer if available
    if (framebuffer)
    {
        EnsureGameTexture(width, height, format);

        // Update texture with framebuffer data
        if (m_GameTexture)
        {
            void* pixels;
            int pitch;
            if (SDL_LockTexture(m_GameTexture, nullptr, &pixels, &pitch))
            {
                // Calculate bytes per pixel based on format
                int bytesPerPixel;
                switch (format)
                {
                    case Deki::ColorFormat::RGB565: bytesPerPixel = 2; break;    // RGB565
                    case Deki::ColorFormat::RGB888: bytesPerPixel = 3; break;    // RGB888
                    case Deki::ColorFormat::ARGB8888: bytesPerPixel = 4; break;  // ARGB8888
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
    // For now, this is a no-op since we're using continuous rendering
    // Could be implemented to invalidate specific texture regions if needed
}

bool SDL3Display::ProcessEvents()
{
    // Event processing is now handled by SDL3Input via DekiInput
    // This method just needs to return true to continue running
    // Quit detection is handled by DekiInput::ShouldExit()
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

    // Set texture blend mode for proper transparency
    SDL_SetTextureBlendMode(overlay, SDL_BLENDMODE_BLEND);

    // Clear with transparent pixels
    SDL_SetRenderTarget(renderer, overlay);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);  // Transparent black
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, nullptr);  // Reset to default target

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

    // Convert RGB565A8 to ARGB8888 for SDL
    // RGB565A8 format: [RGB565_low, RGB565_high, Alpha] per pixel
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

        // Convert RGB565 to RGB888
        uint8_t r = ((rgb565 >> 11) & 0x1F) << 3;  // 5 bits -> 8 bits
        uint8_t g = ((rgb565 >> 5) & 0x3F) << 2;   // 6 bits -> 8 bits
        uint8_t b = (rgb565 & 0x1F) << 3;          // 5 bits -> 8 bits

        // Expand to full 8-bit range (better quality)
        r |= r >> 5;
        g |= g >> 6;
        b |= b >> 5;

        // Pack as ARGB8888
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

        // If this is the active overlay, clear it
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

    // Get texture dimensions
    float w, h;
    SDL_GetTextureSize(m_UiOverlayTexture, &w, &h);

    // Create transparent pixel buffer
    int iw = (int)w, ih = (int)h;
    size_t bufferSize = iw * ih * sizeof(uint32_t);
    // Through the engine like everything else; it already zeroes what it
    // hands back, which is exactly what this buffer is for.
    uint32_t* clearBuffer = (uint32_t*)Deki::Memory::Allocate(bufferSize, Deki::Memory::External);
    if (clearBuffer)
    {
        SDL_UpdateTexture(m_UiOverlayTexture, nullptr, clearBuffer, iw * sizeof(uint32_t));
        Deki::Memory::Free(clearBuffer);
    }
}

}  // namespace DekiSdl3
