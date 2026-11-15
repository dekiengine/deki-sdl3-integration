#include "SDL3Audio.h"

#if DEKI_SDL3_HAS_AUDIO

#include "Mixer.h"  // from deki-audio
#include <SDL3/SDL.h>

#include <algorithm>

namespace DekiSdl3
{

namespace
{
// Frames mixed per pass of the callback, on its stack.
constexpr int kChunkFrames = 512;

void SDLCALL FillStream(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount)
{
    (void)totalAmount;
    const auto* audio = static_cast<const SDL3Audio*>(userdata);
    const int channels = audio->GetChannels();
    const int frameBytes = channels * static_cast<int>(sizeof(int16_t));
    int16_t chunk[kChunkFrames * 2];
    DekiAudio::Mixer& mixer = DekiAudio::Mixer::Instance();
    while (additionalAmount > 0)
    {
        const int frames = std::min(kChunkFrames, (additionalAmount + frameBytes - 1) / frameBytes);
        mixer.Render(chunk, static_cast<size_t>(frames));
        SDL_PutAudioStreamData(stream, chunk, frames * frameBytes);
        additionalAmount -= frames * frameBytes;
    }
}
}  // namespace

SDL3Audio::~SDL3Audio()
{
    Shutdown();
}

void SDL3Audio::Configure(const Deki::PackageConfig& config)
{
    m_SampleRate = config.GetInt("sampleRate", 48000);
    m_Channels = config.GetInt("channels", 2) == 1 ? 1 : 2;
}

bool SDL3Audio::Initialize()
{
    Shutdown();
    if (!SDL_WasInit(SDL_INIT_AUDIO))
    {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        {
            m_LastError = std::string("SDL3Audio: no audio: ") + SDL_GetError();
            m_State = Deki::PackageState::Error;
            return false;
        }
        m_StartedAudio = true;
    }

    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_S16;
    spec.channels = m_Channels;
    spec.freq = m_SampleRate;
    // Opened paused: it starts with the first sound (Start).
    m_Stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &FillStream, this);
    if (!m_Stream)
    {
        m_LastError = std::string("SDL3Audio: no output device: ") + SDL_GetError();
        Shutdown();
        m_State = Deki::PackageState::Error;
        return false;
    }
    m_State = Deki::PackageState::Running;
    return true;
}

void SDL3Audio::Shutdown()
{
    if (m_Stream)
    {
        // Waits for a callback in progress, so the mixer is not read after.
        SDL_DestroyAudioStream(m_Stream);
        m_Stream = nullptr;
    }
    m_Running = false;
    if (m_StartedAudio)
    {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        m_StartedAudio = false;
    }
    m_State = Deki::PackageState::Uninitialized;
}

bool SDL3Audio::Start()
{
    if (!m_Stream)
    {
        m_LastError = "SDL3Audio: no output device";
        return false;
    }
    if (!m_Running && !SDL_ResumeAudioStreamDevice(m_Stream))
    {
        m_LastError = std::string("SDL3Audio: the device did not start: ") + SDL_GetError();
        return false;
    }
    m_Running = true;
    return true;
}

void SDL3Audio::Stop()
{
    if (m_Stream && m_Running)
    {
        SDL_PauseAudioStreamDevice(m_Stream);
    }
    m_Running = false;
}

}  // namespace DekiSdl3

#endif
