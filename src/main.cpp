#include "daisy_seed.h"
#include <atomic>
#include <cstdint>

daisy::DaisySeed seed;
// Share a heartbeat between the audio interrupt and the main loop.
static std::atomic<uint32_t> audio_callbacks{0};
static_assert(ATOMIC_INT_LOCK_FREE == 2, "Audio counter must be lock-free");

void AudioCallback(daisy::AudioHandle::InputBuffer in,
                   daisy::AudioHandle::OutputBuffer out,
                   size_t size)
{
    for(size_t i = 0; i < size; ++i)
    {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
    }
    audio_callbacks.fetch_add(1, std::memory_order_relaxed);
}

int main()
{
    seed.Init();
    seed.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);
    seed.SetAudioBlockSize(48);
    seed.StartLog(false); // USB CDC serial; never wait for a laptop connection.
    seed.StartAudio(AudioCallback);
    seed.PrintLine("Seed3 passthrough started");
    uint32_t previous_callbacks = 0;
    bool led_on = false;
    while(true)
    {
        seed.DelayMs(1000);
        const uint32_t callbacks = audio_callbacks.load(std::memory_order_relaxed);
        const uint32_t delta = callbacks - previous_callbacks;
        previous_callbacks = callbacks;
        led_on = !led_on;
        seed.SetLed(led_on);
        // Repeat configuration so opening the terminal after boot is useful.
        // Print only from the main loop, never from the audio callback.
        seed.PrintLine("Seed3: uptime_ms=%lu sr=%lu block=%lu callbacks=%lu",
                       static_cast<unsigned long>(daisy::System::GetNow()),
                       static_cast<unsigned long>(seed.AudioSampleRate()),
                       static_cast<unsigned long>(seed.AudioBlockSize()),
                       static_cast<unsigned long>(delta));
    }
}
