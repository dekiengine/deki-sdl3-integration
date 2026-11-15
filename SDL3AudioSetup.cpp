#include "SDL3AudioSetup.h"
#include "SDL3Audio.h"
#include <deki/LogSystem.h>

#if DEKI_SDL3_HAS_AUDIO && !defined(DEKI_EDITOR) && defined(DEKI_PACKAGE_SDL3)
#include "AudioOutput.h"    // from deki-audio
#include "DekiAudioInit.h"  // DekiAudioInitSystem (from deki-audio)
#include "Mixer.h"          // from deki-audio
#include <string>
#endif

namespace DekiSdl3
{

#if DEKI_SDL3_HAS_AUDIO && !defined(DEKI_EDITOR) && defined(DEKI_PACKAGE_SDL3)

void SDL3AudioSetup::Setup(SetupCallback onComplete)
{
    // Editor and firmware builds start deki-audio in the generated
    // DekiInitPackageSystems(); the static desktop build links the engine's
    // empty stub for it, so it happens here too. Safe to repeat.
    DekiAudioInitSystem();

    // The mixer first, so it is destroyed after the output that reads it
    // when the game quits.
    DekiAudio::Mixer::Instance();
    static SDL3Audio s_Audio;
    Deki::PackageConfig config;
    config.packageId = "audio";
    config.enabled = true;
    config.settings["sampleRate"] = std::to_string(sampleRate);
    config.settings["channels"] = std::to_string(channels);
    s_Audio.Configure(config);

    const int volume = initialVolume < 0 ? 0 : (initialVolume > 100 ? 100 : static_cast<int>(initialVolume));
    DekiAudio::Mixer::Instance().SetMasterVolume(static_cast<float>(volume) * 0.01f);
    if (s_Audio.Initialize())
    {
        DekiAudio::AudioOutput::SetCurrent(&s_Audio);
    }
    else
    {
        DEKI_LOG_WARNING("SDL3AudioSetup: no sound: %s", s_Audio.GetLastError());
    }
    // A game without sound still starts.
    onComplete(true);
}

#else

// A project without deki-audio has nothing to play, and starts as usual.
void SDL3AudioSetup::Setup(SetupCallback onComplete)
{
    onComplete(true);
}

#endif

}  // namespace DekiSdl3
