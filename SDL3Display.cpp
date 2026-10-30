#include "SDL3Display.h"

#include <algorithm>
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

// One row of the framebuffer to RGBA8. ARGB8888 is a 0xAARRGGBB word per
// pixel, so its bytes in memory are B, G, R, A; RGB565A8 is [565 lo, 565 hi,
// coverage], shown opaque.
void RowToRgba(Deki::ColorFormat format, const uint8_t* src, uint8_t* dst, int32_t count)
{
    for (int32_t x = 0; x < count; ++x)
    {
        uint8_t* d = dst + x * 4;
        switch (format)
        {
            case Deki::ColorFormat::RGB888:
                d[0] = src[x * 3 + 0];
                d[1] = src[x * 3 + 1];
                d[2] = src[x * 3 + 2];
                break;
            case Deki::ColorFormat::ARGB8888:
                d[0] = src[x * 4 + 2];
                d[1] = src[x * 4 + 1];
                d[2] = src[x * 4 + 0];
                break;
            case Deki::ColorFormat::RGB565:
            case Deki::ColorFormat::RGB565A8:
            default:
            {
                const int32_t stride = format == Deki::ColorFormat::RGB565 ? 2 : 3;
                const uint16_t v = static_cast<uint16_t>(src[x * stride] | (src[x * stride + 1] << 8));
                const uint8_t r5 = (v >> 11) & 0x1F, g6 = (v >> 5) & 0x3F, b5 = v & 0x1F;
                d[0] = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
                d[1] = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
                d[2] = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
                break;
            }
        }
        d[3] = 255;
    }
}

#ifndef _WIN32
// The window's icon where the executable carries none (Linux, macOS): the Deki
// cat peeking over its ledge, as in the editor's title bar. On
// Windows the exe's icon (deki-desktop-integration's resources/windows) is
// used at every size instead.
void SetCatIcon(SDL_Window* window)
{
    static const char* const kCat[] = {
        "................", "................", "................", "...A........A...",
        "...AA......AA...", "...AAAAAAAAAA...", "..AAAAAAAAAAAA..", "..AAKSAAAAKSAA..",
        "..AAKKAAAAKKAA..", "..AAAAAPPAAAAA..", "..AWWAAAAAAWWA..", "BBBBBBBBBBBBBBBB",
        "................", "................", "................", "................",
    };
    constexpr int kScale = 4;  // 64 px, by whole pixels
    SDL_Surface* icon = SDL_CreateSurface(16 * kScale, 16 * kScale, SDL_PIXELFORMAT_RGBA32);
    if (!icon)
    {
        return;
    }
    for (int y = 0; y < 16 * kScale; ++y)
    {
        uint8_t* row = static_cast<uint8_t*>(icon->pixels) + static_cast<size_t>(y) * icon->pitch;
        for (int x = 0; x < 16 * kScale; ++x)
        {
            uint8_t rgba[4] = { 0, 0, 0, 0 };
            switch (kCat[y / kScale][x / kScale])
            {
                case 'A': rgba[0] = 0x3b, rgba[1] = 0xc3, rgba[2] = 0xff, rgba[3] = 255; break;  // fur
                case 'W': rgba[0] = 0x9d, rgba[1] = 0xe1, rgba[2] = 0xff, rgba[3] = 255; break;  // paws
                case 'B': rgba[0] = 0x0c, rgba[1] = 0x82, rgba[2] = 0xbf, rgba[3] = 255; break;  // ledge
                case 'S': rgba[0] = 0xe8, rgba[1] = 0xe6, rgba[2] = 0xe1, rgba[3] = 255; break;  // eye shine
                case 'K': rgba[0] = 0x0a, rgba[1] = 0x0b, rgba[2] = 0x0d, rgba[3] = 255; break;  // eyes
                case 'P': rgba[0] = 0xff, rgba[1] = 0x8f, rgba[2] = 0xb8, rgba[3] = 255; break;  // mouth
                default: break;
            }
            std::memcpy(row + x * 4, rgba, 4);
        }
    }
    SDL_SetWindowIcon(window, icon);
    SDL_DestroySurface(icon);
}
#endif

SDL_GPUDevice* CreateDevice()
{
    // No shaders are used, but SDL wants the formats the program could give.
    const SDL_GPUShaderFormat formats = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXBC |
                                        SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL |
                                        SDL_GPU_SHADERFORMAT_METALLIB;
#ifdef _WIN32
    // SDL would pick Vulkan first; Direct3D 12 is the system's own.
    if (SDL_GPUDevice* device = SDL_CreateGPUDevice(formats, false, "direct3d12"))
    {
        return device;
    }
#endif
    return SDL_CreateGPUDevice(formats, false, nullptr);
}
}  // namespace

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
#ifndef _WIN32
    SetCatIcon(window);
#endif

    m_Device = CreateDevice();
    if (m_Device == nullptr || !SDL_ClaimWindowForGPUDevice(m_Device, window))
    {
        DEKI_LOG_ERROR("SDL could not start the graphics device: %s", SDL_GetError());
        if (m_Device)
        {
            SDL_DestroyGPUDevice(m_Device);
            m_Device = nullptr;
        }
        SDL_DestroyWindow(window);
        window = nullptr;
        SDL_Quit();
        return false;
    }

    // No vsync: a present that waits for vblank blocks for seconds when the
    // window stops getting them (hidden or in the background), and on Intel
    // integrated GPUs a waiting DXGI present could block forever. The engine's
    // own frame limiter (Deki::Time::Delay and the target FPS) paces frames,
    // and Draw never waits for a window image.
    if (SDL_WindowSupportsGPUPresentMode(m_Device, window, SDL_GPU_PRESENTMODE_IMMEDIATE))
    {
        SDL_SetGPUSwapchainParameters(m_Device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                      SDL_GPU_PRESENTMODE_IMMEDIATE);
    }
    else if (SDL_WindowSupportsGPUPresentMode(m_Device, window, SDL_GPU_PRESENTMODE_MAILBOX))
    {
        SDL_SetGPUSwapchainParameters(m_Device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_MAILBOX);
    }

    initialized = true;
    DEKI_LOG_INTERNAL("SDL3 display initialized: %dx%d screen, window x%d, %s", width, height, (int)m_WindowScale,
                      SDL_GetGPUDeviceDriver(m_Device));

    return true;
}

void SDL3Display::Shutdown()
{
    if (!initialized)
    {
        return;
    }

    if (m_GameTexture)
    {
        SDL_ReleaseGPUTexture(m_Device, m_GameTexture);
        m_GameTexture = nullptr;
    }
    if (m_Upload)
    {
        SDL_ReleaseGPUTransferBuffer(m_Device, m_Upload);
        m_Upload = nullptr;
        m_UploadSize = 0;
    }
    SDL_ReleaseWindowFromGPUDevice(m_Device, window);
    SDL_DestroyGPUDevice(m_Device);
    m_Device = nullptr;

    SDL_DestroyWindow(window);
    window = nullptr;

    SDL_Quit();
    initialized = false;
}

void SDL3Display::EnsureGameTexture(int width, int height)
{
    if (m_GameTexture && width == m_LastFbWidth && height == m_LastFbHeight)
    {
        return;
    }

    if (m_GameTexture)
    {
        SDL_ReleaseGPUTexture(m_Device, m_GameTexture);
        m_GameTexture = nullptr;
    }

    // A colour target too, so Draw can clear it.
    SDL_GPUTextureCreateInfo texture{};
    texture.type = SDL_GPU_TEXTURETYPE_2D;
    texture.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    texture.width = static_cast<Uint32>(width);
    texture.height = static_cast<Uint32>(height);
    texture.layer_count_or_depth = 1;
    texture.num_levels = 1;
    m_GameTexture = SDL_CreateGPUTexture(m_Device, &texture);
    if (!m_GameTexture)
    {
        DEKI_LOG_ERROR("SDL3Display: no %dx%d game texture: %s", width, height, SDL_GetError());
    }
    m_ClearTexture = true;
    m_LastFbWidth = width;
    m_LastFbHeight = height;
}

void SDL3Display::Draw(const uint8_t* framebuffer, int width, int height, Deki::ColorFormat format,
                       const Deki::Rect* rects, int32_t count)
{
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(m_Device);
    if (cmd == nullptr)
    {
        DEKI_LOG_WARNING("SDL3Display: no command buffer: %s", SDL_GetError());
        return;
    }

    // A new texture starts black, before anything is uploaded into it.
    if (m_ClearTexture && m_GameTexture)
    {
        SDL_GPUColorTargetInfo target{};
        target.texture = m_GameTexture;
        target.load_op = SDL_GPU_LOADOP_CLEAR;
        target.store_op = SDL_GPU_STOREOP_STORE;
        target.clear_color = SDL_FColor{ 0.0f, 0.0f, 0.0f, 1.0f };
        SDL_EndGPURenderPass(SDL_BeginGPURenderPass(cmd, &target, 1, nullptr));
        m_ClearTexture = false;
    }

    // The rectangles, inside the frame, packed one after another in the
    // upload buffer, widened to RGBA8.
    size_t pixels = 0;
    for (int32_t i = 0; framebuffer && i < count; ++i)
    {
        const Deki::Rect& r = rects[i];
        const int32_t w = std::min(r.right, width) - std::max(r.left, 0);
        const int32_t h = std::min(r.bottom, height) - std::max(r.top, 0);
        pixels += (w > 0 && h > 0) ? static_cast<size_t>(w) * h : 0;
    }
    if (pixels > 0 && m_GameTexture && m_UploadSize < pixels * 4)
    {
        if (m_Upload)
        {
            SDL_ReleaseGPUTransferBuffer(m_Device, m_Upload);
        }
        SDL_GPUTransferBufferCreateInfo upload{};
        upload.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        upload.size = static_cast<Uint32>(pixels * 4);
        m_Upload = SDL_CreateGPUTransferBuffer(m_Device, &upload);
        m_UploadSize = m_Upload ? upload.size : 0;
    }
    uint8_t* staging = (pixels > 0 && m_GameTexture && m_Upload)
                           ? static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(m_Device, m_Upload, true))
                           : nullptr;
    if (staging)
    {
        const size_t bpp = Deki::FrameBufferBytes(format, 1, 1);
        SDL_GPUTextureTransferInfo source{};
        source.transfer_buffer = m_Upload;
        SDL_GPUTextureRegion destination{};
        destination.texture = m_GameTexture;
        destination.d = 1;
        uint8_t* out = staging;
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
        for (int32_t i = 0; i < count; ++i)
        {
            const int32_t left = std::max(rects[i].left, 0);
            const int32_t top = std::max(rects[i].top, 0);
            const int32_t w = std::min(rects[i].right, width) - left;
            const int32_t h = std::min(rects[i].bottom, height) - top;
            if (w <= 0 || h <= 0)
            {
                continue;
            }
            source.offset = static_cast<Uint32>(out - staging);
            source.pixels_per_row = static_cast<Uint32>(w);
            source.rows_per_layer = static_cast<Uint32>(h);
            for (int32_t y = top; y < top + h; ++y)
            {
                RowToRgba(format, framebuffer + (static_cast<size_t>(y) * width + left) * bpp, out, w);
                out += static_cast<size_t>(w) * 4;
            }
            destination.x = static_cast<Uint32>(left);
            destination.y = static_cast<Uint32>(top);
            destination.w = static_cast<Uint32>(w);
            destination.h = static_cast<Uint32>(h);
            SDL_UploadToGPUTexture(copy, &source, &destination, false);
        }
        SDL_UnmapGPUTransferBuffer(m_Device, m_Upload);
        SDL_EndGPUCopyPass(copy);
    }

    // The window image, when there is one: none while minimized, or while
    // earlier frames are still on their way to the screen.
    SDL_GPUTexture* swapchain = nullptr;
    Uint32 windowWidth = 0;
    Uint32 windowHeight = 0;
    if (SDL_AcquireGPUSwapchainTexture(cmd, window, &swapchain, &windowWidth, &windowHeight) && swapchain &&
        m_GameTexture)
    {
        // Whole pixels, centred, like a device panel; a window smaller than
        // the screen shows all of it, shrunk.
        const int32_t w = m_LastFbWidth;
        const int32_t h = m_LastFbHeight;
        const int32_t scale = std::min(static_cast<int32_t>(windowWidth) / w, static_cast<int32_t>(windowHeight) / h);
        int32_t drawWidth = w * scale;
        int32_t drawHeight = h * scale;
        if (scale < 1)
        {
            const float fit = std::min(static_cast<float>(windowWidth) / w, static_cast<float>(windowHeight) / h);
            drawWidth = std::max(1, static_cast<int32_t>(w * fit));
            drawHeight = std::max(1, static_cast<int32_t>(h * fit));
        }

        SDL_GPUBlitInfo blit{};
        blit.source.texture = m_GameTexture;
        blit.source.w = static_cast<Uint32>(w);
        blit.source.h = static_cast<Uint32>(h);
        blit.destination.texture = swapchain;
        blit.destination.x = static_cast<Uint32>((static_cast<int32_t>(windowWidth) - drawWidth) / 2);
        blit.destination.y = static_cast<Uint32>((static_cast<int32_t>(windowHeight) - drawHeight) / 2);
        blit.destination.w = static_cast<Uint32>(drawWidth);
        blit.destination.h = static_cast<Uint32>(drawHeight);
        blit.load_op = SDL_GPU_LOADOP_CLEAR;
        blit.clear_color = SDL_FColor{ 0.0f, 0.0f, 0.0f, 1.0f };
        blit.filter = SDL_GPU_FILTER_NEAREST;
        SDL_BlitGPUTexture(cmd, &blit);
    }

    SDL_SubmitGPUCommandBuffer(cmd);
}

void SDL3Display::Present(const uint8_t* framebuffer, int width, int height, Deki::ColorFormat format)
{
    if (!initialized)
    {
        return;
    }

    if (framebuffer)
    {
        EnsureGameTexture(width, height);
    }
    const Deki::Rect whole{ 0, 0, width, height };
    Draw(framebuffer, width, height, format, &whole, 1);
}

void SDL3Display::PresentRegions(const uint8_t* framebuffer, int width, int height, Deki::ColorFormat format,
                                 const Deki::Rect* rects, int32_t count)
{
    if (!initialized)
    {
        return;
    }

    if (framebuffer)
    {
        EnsureGameTexture(width, height);
    }
    // The window is redrawn from the texture every call, changed or not.
    Draw(framebuffer, width, height, format, rects, count);
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

}  // namespace DekiSdl3
