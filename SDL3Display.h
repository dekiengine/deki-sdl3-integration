#pragma once

#include <deki/Engine.h>  // ColorFormat
#include <SDL3/SDL.h>

#include <deki/providers/IDisplay.h>

namespace DekiSdl3
{

/**
 * @brief SDL3 implementation of display interface
 */
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

    // Game texture cache variables
    int last_fb_width, last_fb_height;

    // The format the current texture was built for. Only consulted when
    // m_GameTexture is non-null, so it needs no "nothing yet" sentinel - which
    // is what the old int -1 was, and what a ColorFormat cannot express.
    Deki::ColorFormat m_LastFbFormat = Deki::ColorFormat::RGB565;

   public:
    SDL3Display();
    virtual ~SDL3Display();

    // Window pixels per screen pixel. Set before Initialize; 1 by default.
    void SetWindowScale(int32_t scale) { m_WindowScale = scale > 0 ? scale : 1; }

    // IPlatformDisplay interface. width x height is the emulated screen: what
    // GetDisplaySize reports and the framebuffer is sized to. The window is
    // that times the window scale, with whole-pixel upscaling.
    bool Initialize(int32_t width, int32_t height) override;
    void Shutdown() override;
    void Present(const uint8_t* framebuffer, int width, int height, Deki::ColorFormat format) override;
    bool SupportsPartialPresent() const override { return true; }
    void PresentRegions(const uint8_t* framebuffer, int width, int height, Deki::ColorFormat format,
                        const Deki::Rect* rects, int32_t count) override;
    void GetDisplaySize(int32_t* width, int32_t* height) const override;
    bool IsInitialized() const override;
    void RequestFullRefresh() override;
    bool ProcessEvents() override;

    // UI/Overlay support
    void* CreateUIOverlay(int32_t width, int32_t height) override;
    bool UpdateUIOverlay(
        void* overlay, int32_t x, int32_t y, int32_t width, int32_t height, const uint32_t* pixels) override;
    bool UpdateUIOverlayRGB565A8(
        void* overlay, int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t* rgb565a8_pixels) override;
    void DestroyUIOverlay(void* overlay) override;
    void SetActiveUIOverlay(void* overlay) override;
    void ClearActiveUIOverlay() override;

    // SDL3-specific methods
    SDL_Renderer* GetRenderer() const { return renderer; }
    SDL_Window* GetWindow() const { return window; }

   private:
    // (Re)create the game texture for this frame size/format. Returns true
    // when it was recreated (contents undefined: upload the whole frame).
    bool EnsureGameTexture(int width, int height, Deki::ColorFormat format);
    // Clear, draw the game texture and the overlay, present the window.
    void DrawWindow();
};

}  // namespace DekiSdl3
