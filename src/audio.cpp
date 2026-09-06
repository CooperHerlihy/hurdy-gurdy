#include "hg/audio.hpp"

namespace hg {

template<>
void assetLoadImpl(AssetData<Sound>* data)
{
    static_cast<void>(data);
    HG_PANIC("Load audio file impl : TODO\n");
}

void AudioPlayer::update(Span<f32> buf, AudioConfig config)
{
    SpinLockScope scope{lock};

    (void)config;

    for (u32 i = 0; i < sounds.count; ++i)
    {
        for (f32& f : buf)
        {
            f += sounds[i].asset->data[sounds[i].pos++] * sounds[i].gain;
            if (sounds[i].pos == sounds[i].asset->data.count)
            {
                sounds.removeSwap(i--);
                break;
            }
        }
    }

    for (u32 i = 0; i < music.count; ++i)
    {
        if (!music[i].playing)
            continue;

        for (f32& f : buf)
        {
            f += music[i].asset->data[music[i].pos++] * music[i].gain;
            if (music[i].pos == music[i].asset->data.count)
            {
                music[i].pos -= music[i].asset->data.count;
            }
        }
    }
}

void AudioPlayer::playSound(const Asset<Sound>& soundSrc, f32 gain)
{
    SpinLockScope scope{lock};

    sounds.push({soundSrc.clone(), 0, gain, true});
}

void AudioPlayer::playMusic(const Asset<Sound>& musicSrc, f32 gain)
{
    SpinLockScope scope{lock};

    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].asset == musicSrc)
        {
            music[i].playing = true;
            return;
        }
    }

    music.push({musicSrc.clone(), 0, gain, true});
}

void AudioPlayer::killMusic(const Asset<Sound>& musicSrc)
{
    SpinLockScope scope{lock};

    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].asset == musicSrc)
        {
            music.removeShift(i);
            return;
        }
    }
}

void AudioPlayer::pauseMusic(const Asset<Sound>& musicSrc)
{
    SpinLockScope scope{lock};

    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].asset == musicSrc)
        {
            music[i].playing = false;
            return;
        }
    }
}

void AudioPlayer::setMusicGain(const Asset<Sound>& musicSrc, f32 gain)
{
    SpinLockScope scope{lock};

    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].asset == musicSrc)
        {
            music[i].gain = gain;
            return;
        }
    }
}

} // namespace hg
