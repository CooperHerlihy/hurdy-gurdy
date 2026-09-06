#pragma once

#include "hg/inttypes.hpp"
#include "hg/array.hpp"
#include "hg/assets.hpp"
#include "hg/concurrency.hpp"

namespace hg {

/**
 * The config for the audio stream
 */
struct AudioConfig {
    /**
     * The number of channels (mono, stereo, etc.)
     */
    u32 channels = 1;
    /**
     * The samples per second
     */
    u32 sampleRate = 48000;
};

/**
 * The callback used by the audio thread
 *
 * Parameters
 * - userData The custom user data
 * - audioBuffer The buffer that needs to be filled by the implementation
 * - config The configuration for the audio stream, should be but may not be the
 *   same as preferredConfig from setCallback
 */
using AudioCallback = void (*)(void* userData, Span<f32> audioBuffer, AudioConfig config);

/**
 * An audio device
 */
struct AudioDevice {
    /**
     * The audio device identifier
     */
    u32 id;

    /**
     * Set the current audio callback and start the device
     */
    void setCallback(AudioCallback callback, void* userData, const AudioConfig& preferredConfig = {});

    /**
     * Remove the callback and stop the device
     */
    void unsetCallback();
};

/**
 * Get the default audio device
 */
AudioDevice& defaultAudioDevice();

/**
 * Audio data asset
 */
struct Sound {
    /**
     * The sound data
     */
    Span<f32> data{};
    /**
     * The floats per second
     */
    u32 frequency = 0;
    /**
     * The number of channels (mono, stereo, etc.)
     */
    u32 channels = 0;
};

/**
 * AudioData asset load implementation
 */
template<>
void assetLoadImpl(AssetData<Sound>* data);

/**
 * A music track in the audio player
 */
struct AudioPlayerSound {
    /**
     * The sound to play
     */
    Asset<Sound> asset{};
    /**
     * The current position in the sound
     */
    u64 pos = 0;
    /**
     * The current gain
     */
    f32 gain = 0.0f;
    /**
     * Whether the music is currently playing or paused
     */
    bool playing = false;
};

/**
 * An audio player system
 */
struct AudioPlayer {
    /**
     * The mutex
     */
    SpinLock lock{};
    /**
     * The temporary sounds
     */
    Array<AudioPlayerSound> sounds{};
    /**
     * The repeating music
     */
    Array<AudioPlayerSound> music{};

    /**
     * Update the music and sounds
     */
    void update(Span<f32> buf, AudioConfig config);

    /**
     * Update the music and sounds
     */
    static void callback(void* player, Span<f32> buf, AudioConfig config)
    {
        (*static_cast<AudioPlayer*>(player)).update(buf, config);
    }

    /**
     * Play a sound once
     */
    void playSound(const Asset<Sound>& sound, f32 gain = 1.0f);

    /**
     * Start a new music track, or resume an existing one
     */
    void playMusic(const Asset<Sound>& music, f32 gain = 1.0f);

    /**
     * Remove a music track from the player
     */
    void killMusic(const Asset<Sound>& music);

    /**
     * Pause a music track
     */
    void pauseMusic(const Asset<Sound>& music);

    /**
     * Set the volume for a music track
     */
    void setMusicGain(const Asset<Sound>& music, f32 gain);
};

} // namespace hg

