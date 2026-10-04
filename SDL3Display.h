#pragma once

#include <deki/Engine.h>  // ColorFormat
#include <SDL3/SDL.h>

#include <deki/providers/IDisplay.h>

namespace DekiSdl3
{

/// A display shown in an SDL3 window.
class SDL3Display : public Deki::IDisplay
{
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* m_GameTexture;
    SDL_Texture* m_UiOverlayTexture;

    int32_t m_DisplayWidth;
    int32_t m_DisplayHeight;
    bool initialized;

    int32_t m_WindowScale = 1;
    Deki::ColorFormat m_Format = Deki::ColorFormat::RGB565;

    // Size of the current game texture.
    int m_LastFbWidth, m_LastFbHeight;

    // The format the current texture was built for. Only read when
    // m_GameTexture is non-null, so it needs no "nothing yet" value.
    Deki::ColorFormat m_LastFbFormat = Deki::ColorFormat::RGB565;

public:
    SDL3Display();
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

    void* CreateUIOverlay(int32_t width, int32_t height) override;
    bool UpdateUIOverlay(void* overlay, int32_t x, int32_t y, int32_t width, int32_t height,
                         const uint32_t* pixels) override;
    bool UpdateUIOverlayRGB565A8(void* overlay, int32_t x, int32_t y, int32_t width, int32_t height,
                                 const uint8_t* rgb565a8Pixels) override;
    void DestroyUIOverlay(void* overlay) override;
    void SetActiveUIOverlay(void* overlay) override;
    void ClearActiveUIOverlay() override;

    SDL_Renderer* GetRenderer() const { return renderer; }
    SDL_Window* GetWindow() const { return window; }

private:
    // Creates the game texture again when the frame's size or format changed.
    // Returns true when it did; the new texture's contents are undefined, so
    // the whole frame must be uploaded.
    bool EnsureGameTexture(int width, int height, Deki::ColorFormat format);
    // Clears, draws the game texture and the overlay, and presents the window.
    void DrawWindow();
};

}  // namespace DekiSdl3
