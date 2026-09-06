#include "daisy_seed.h"

daisy::DaisySeed seed;

void AudioCallback(daisy::AudioHandle::InputBuffer in,
                   daisy::AudioHandle::OutputBuffer out,
                   size_t size)
{
    for(size_t i = 0; i < size; ++i)
    {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
    }
}

int main()
{
    seed.Init();
    seed.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);
    seed.SetAudioBlockSize(48);
    seed.StartAudio(AudioCallback);
    while(true)
        seed.DelayMs(100);
}
