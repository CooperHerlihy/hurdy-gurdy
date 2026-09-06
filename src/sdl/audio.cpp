#include "hg/audio.hpp"

#include "internal.hpp"
#include "hg/error.hpp"

#include <SDL3/SDL_audio.h>

namespace hg {

struct AudioState {
    SDL_AudioDeviceID device = 0;
    SDL_AudioStream* stream = nullptr;

    AudioCallback callback = nullptr;
    void* callbackData = nullptr;
    AudioConfig callbackConfig{};

    AudioDevice defaultAudioDevice = {0};
};

static AudioState audio{};

bool internal::initAudio()
{
    audio.device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (audio.device == 0)
    {
        setError("SDL could not open audio device: %s", SDL_GetError());
        return false;
    }

    audio.stream = SDL_CreateAudioStream(nullptr, nullptr);
    if (audio.stream == nullptr)
    {
        setError("SDL could not open audio stream: %s", SDL_GetError());
        SDL_CloseAudioDevice(audio.device);
        return false;
    }

    if (!SDL_BindAudioStream(audio.device, audio.stream))
    {
        setError("SDL could not bind audio stream to device: %s", SDL_GetError());
        SDL_DestroyAudioStream(audio.stream);
        SDL_CloseAudioDevice(audio.device);
        return false;
    }

    return true;
}

void internal::deinitAudio()
{
    SDL_DestroyAudioStream(audio.stream);
    SDL_CloseAudioDevice(audio.device);
}

void sdlCallback(
    void* userData,
    SDL_AudioStream* stream,
    int additionalAmount,
    int totalAmount)
{
    (void)userData;
    (void)totalAmount;

    ArenaScope scratch = getScratch();

    void* buf = scratch.alloc(static_cast<u64>(additionalAmount), alignof(f32));
    memset(buf, 0, static_cast<u64>(additionalAmount));

    audio.callback(
        audio.callbackData,
        {static_cast<f32*>(buf), static_cast<u64>(additionalAmount) / sizeof(f32)},
        audio.callbackConfig);

    if (!SDL_PutAudioStreamData(stream, buf, additionalAmount))
        HG_PANIC("SDL could not push audio stream data: %s\n", SDL_GetError());
}

void AudioDevice::setCallback(AudioCallback callback, void* userData, const AudioConfig& preferredConfig)
{
    SDL_AudioSpec audioSpec{};
    audioSpec.format = SDL_AUDIO_F32;
    audioSpec.freq = static_cast<int>(preferredConfig.sampleRate);
    audioSpec.channels = static_cast<int>(preferredConfig.channels);

    if (!SDL_SetAudioStreamFormat(audio.stream, &audioSpec, nullptr))
        HG_PANIC("SDL could not set audio stream format: %s\n", SDL_GetError());

    audio.callback = callback;
    audio.callbackData = userData;
    audio.callbackConfig = preferredConfig;

    if (!SDL_SetAudioStreamGetCallback(audio.stream, sdlCallback, nullptr))
        HG_PANIC("SDL could not set audio stream callback: %s\n", SDL_GetError());
}

void AudioDevice::unsetCallback()
{
    audio.callback = nullptr;
    audio.callbackData = nullptr;
    audio.callbackConfig = {};
}

AudioDevice& defaultAudioDevice()
{
    return audio.defaultAudioDevice;
}

} // namespace hg
