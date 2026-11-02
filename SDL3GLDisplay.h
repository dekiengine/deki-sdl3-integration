#pragma once

// An SDL3 window shown through OpenGL, by deki-gl's presenter: the frame is
// built on the GPU from the 2D renderer's layers and anything that draws into
// it directly (deki-3d-gl's meshes), then scaled into the window. Built only
// where the project has deki-gl; SDL3Display, over SDL's GPU API, is the
// display everywhere else and the fallback when OpenGL does not come up.

#if defined(DEKI_PACKAGE_GL) && !defined(DEKI_EDITOR)

#include <deki/Engine.h>  // ColorFormat
#include <deki/providers/IDisplay.h>
#include <SDL3/SDL.h>

#include <memory>

#include "GLApi.h"        // from deki-gl
#include "GLPresenter.h"  // from deki-gl

namespace DekiSdl3
{

class SDL3GLDisplay : public Deki::IDisplay
{
public:
    SDL3GLDisplay() = default;
    ~SDL3GLDisplay() override;

    /// Window pixels per screen pixel. Set before Initialize; 1 by default.
    void SetWindowScale(int32_t scale) { m_WindowScale = scale > 0 ? scale : 1; }
    /// Whether the presenter draws the 2D too (GLPresenter::SetDraws2D).
    /// Set before Initialize; on by default.
    void SetDraw2DOnGpu(bool draw) { m_Draw2DOnGpu = draw; }

    // IDisplay. width x height is the emulated screen, as for SDL3Display.
    // False when OpenGL does not come up; the window is closed again.
    bool Initialize(int32_t width, int32_t height) override;
    void Shutdown() override;
    void Present(const uint8_t* framebuffer, int width, int height, Deki::ColorFormat format) override;
    void GetDisplaySize(int32_t* width, int32_t* height) const override;
    // RGB565A8 whatever the platform's screen format: the frame buffer must
    // keep coverage for the presenter to compose it in layers.
    Deki::ColorFormat GetColorFormat() const override { return Deki::ColorFormat::RGB565A8; }
    bool IsInitialized() const override { return m_Presenter != nullptr; }
    void RequestFullRefresh() override {}
    bool ProcessEvents() override { return true; }

    // Overlays are not used here: nothing in the engine creates one.
    void* CreateUIOverlay(int32_t, int32_t) override { return nullptr; }
    bool UpdateUIOverlay(void*, int32_t, int32_t, int32_t, int32_t, const uint32_t*) override { return false; }
    bool UpdateUIOverlayRGB565A8(void*, int32_t, int32_t, int32_t, int32_t, const uint8_t*) override { return false; }
    void DestroyUIOverlay(void*) override {}
    void SetActiveUIOverlay(void*) override {}
    void ClearActiveUIOverlay() override {}

    SDL_Window* GetWindow() const { return m_Window; }

private:
    SDL_Window* m_Window = nullptr;
    SDL_GLContext m_GLContext = nullptr;
    std::shared_ptr<DekiGL::GLContext> m_Context;
    std::unique_ptr<DekiGL::GLPresenter> m_Presenter;
    int32_t m_Width = 0;
    int32_t m_Height = 0;
    int32_t m_WindowScale = 1;
    bool m_Draw2DOnGpu = true;
};

}  // namespace DekiSdl3

#endif
