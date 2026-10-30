#pragma once

#include <deki/Engine.h>  // ColorFormat
#include <SDL3/SDL.h>

#include <deki/providers/IDisplay.h>

namespace DekiSdl3
{

/// A display shown in an SDL3 window, drawn with SDL's GPU API (Direct3D 12
/// on Windows, Vulkan on Linux, Metal on macOS).
class SDL3Display : public Deki::IDisplay
{
private:
    SDL_Window* window = nullptr;
    SDL_GPUDevice* m_Device = nullptr;
    SDL_GPUTexture* m_GameTexture = nullptr;
    SDL_GPUTransferBuffer* m_Upload = nullptr;  // the rectangles of one present, RGBA8
    uint32_t m_UploadSize = 0;
    bool m_ClearTexture = false;  // a new game texture, cleared before its first use

    int32_t m_DisplayWidth = 0;
    int32_t m_DisplayHeight = 0;
    bool initialized = false;

    int32_t m_WindowScale = 1;
    Deki::ColorFormat m_Format = Deki::ColorFormat::RGB565;

    // Size of the current game texture.
    int m_LastFbWidth = 0;
    int m_LastFbHeight = 0;

public:
    SDL3Display() = default;
    virtual ~SDL3Display();

    /// Window pixels per screen pixel. Set before Initialize; 1 by default.
    void SetWindowScale(int32_t scale) { m_WindowScale = scale > 0 ? scale : 1; }
    /// The emulated screen's pixel format. Set before Initialize.
    void SetColorFormat(Deki::ColorFormat format) { m_Format = format; }

    // IDisplay. width x height is the emulated screen: what GetDisplaySize
    // reports and the framebuffer's size. The window is that times the window
    // scale, upscaled by whole pixels.
    bool Initialize(int32_t width, int32_t height) override;
    void Shutdown() override;
    void Present(const uint8_t* framebuffer, int width, int height, Deki::ColorFormat format) override;
    bool SupportsPartialPresent() const override { return true; }
    void PresentRegions(const uint8_t* framebuffer, int width, int height, Deki::ColorFormat format,
                        const Deki::Rect* rects, int32_t count) override;
    void GetDisplaySize(int32_t* width, int32_t* height) const override;
    Deki::ColorFormat GetColorFormat() const override { return m_Format; }
    bool IsInitialized() const override;
    void RequestFullRefresh() override;
    bool ProcessEvents() override;

    // Overlays are not used here: nothing in the engine creates one.
    void* CreateUIOverlay(int32_t, int32_t) override { return nullptr; }
    bool UpdateUIOverlay(void*, int32_t, int32_t, int32_t, int32_t, const uint32_t*) override { return false; }
    bool UpdateUIOverlayRGB565A8(void*, int32_t, int32_t, int32_t, int32_t, const uint8_t*) override { return false; }
    void DestroyUIOverlay(void*) override {}
    void SetActiveUIOverlay(void*) override {}
    void ClearActiveUIOverlay() override {}

    SDL_Window* GetWindow() const { return window; }

private:
    // Creates the game texture again when the frame's size changed; the new
    // one starts black.
    void EnsureGameTexture(int width, int height);
    // Uploads the rectangles of the frame (widened to RGBA8), draws the game
    // texture into the window by whole pixels and presents it. Reads only the
    // rectangles' pixels: a renderer drawing in strips hands over a frame
    // whose other rows do not exist.
    void Draw(const uint8_t* framebuffer, int width, int height, Deki::ColorFormat format, const Deki::Rect* rects,
              int32_t count);
};

}  // namespace DekiSdl3
