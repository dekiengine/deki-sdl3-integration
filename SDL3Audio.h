#pragma once

// The computer's sound card as deki-audio's output, through SDL3: on Windows,
// Linux and macOS, in the browser, and in the editor's Play view. Built only
// when the project has deki-audio.

#if __has_include("IDekiAudio.h")
#define DEKI_SDL3_HAS_AUDIO 1

#include "IDekiAudio.h"  // from deki-audio
#include <deki/PackageConfig.h>
#include <string>

struct SDL_AudioStream;

namespace DekiSdl3
{

/// Plays the mixer through SDL's default output device. SDL asks for samples
/// from its own audio thread (on the web, from the page's audio callback) and
/// converts them to what the device takes.
class SDL3Audio : public DekiAudio::IDekiAudio
{
public:
    SDL3Audio() = default;
    ~SDL3Audio() override;

    const char* GetPackageId() const override { return "audio"; }
    const char* GetPackageName() const override { return "SDL3 Audio"; }
    /// Reads "sampleRate" (48000) and "channels" (2).
    void Configure(const Deki::PackageConfig& config) override;
    /// Opens the default output device, paused.
    bool Initialize() override;
    void Shutdown() override;
    void Update(float) override {}
    Deki::PackageState GetState() const override { return m_State; }
    const char* GetLastError() const override { return m_LastError.c_str(); }

    int GetSampleRate() const override { return m_SampleRate; }
    int GetChannels() const override { return m_Channels; }
    bool Start() override;
    void Stop() override;
    bool IsRunning() const override { return m_Running; }
    bool IsHardwareConnected() const override { return m_Stream != nullptr; }

private:
    SDL_AudioStream* m_Stream = nullptr;
    int m_SampleRate = 48000;
    int m_Channels = 2;
    bool m_Running = false;
    bool m_StartedAudio = false;
    Deki::PackageState m_State = Deki::PackageState::Uninitialized;
    std::string m_LastError;
};

}  // namespace DekiSdl3

#endif
