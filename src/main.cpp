#include "daisy_seed.h"
#include "nam_audio.h"
#if NAM_ENABLE_LOGGING
#include <atomic>
#include <cstdint>
#endif

daisy::DaisySeed seed;
#if NAM_ENABLE_LOGGING
// Share a heartbeat between the audio interrupt and the main loop.
static std::atomic<uint32_t> audio_callbacks{0};
static_assert(ATOMIC_INT_LOCK_FREE == 2, "Audio counter must be lock-free");
#endif

static NamAudio audio;
// Set true and rebuild for a clean left-input comparison at the same output
// gain.
constexpr bool kBypass = false;

void AudioCallback(daisy::AudioHandle::InputBuffer in,
                   daisy::AudioHandle::OutputBuffer out, size_t size) {
  audio.Process(in[0], out[0], out[1], size, kBypass);
#if NAM_ENABLE_LOGGING
  audio_callbacks.fetch_add(1, std::memory_order_relaxed);
#endif
}

int main() {
  seed.Init();
  seed.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);
  seed.SetAudioBlockSize(NamAudio::kBlockSize);
#if NAM_ENABLE_LOGGING
  seed.StartLog(false); // USB CDC serial; never wait for a laptop connection.
#endif
  [[maybe_unused]] const bool model_ready = audio.Init();
  seed.StartAudio(AudioCallback);
#if NAM_ENABLE_LOGGING
  seed.PrintLine(
      "Seed3 NAM Test LSTM: model=%s bypass=%u input_gain=1 output_gain=0.8",
      model_ready ? "ready" : "failed",
      static_cast<unsigned>(kBypass || !model_ready));
  uint32_t previous_callbacks = 0;
#endif
  bool led_on = false;
  while (true) {
    seed.DelayMs(1000);
#if NAM_ENABLE_LOGGING
    const uint32_t callbacks = audio_callbacks.load(std::memory_order_relaxed);
    const uint32_t delta = callbacks - previous_callbacks;
    previous_callbacks = callbacks;
#endif
    led_on = !led_on;
    seed.SetLed(led_on);
#if NAM_ENABLE_LOGGING
    // Repeat configuration so opening the terminal after boot is useful.
    // Print only from the main loop, never from the audio callback.
    seed.PrintLine("Seed3 NAM: model=%s bypass=%u uptime_ms=%lu sr=%lu "
                   "block=%lu callbacks=%lu",
                   model_ready ? "ready" : "failed",
                   static_cast<unsigned>(kBypass || !model_ready),
                   static_cast<unsigned long>(daisy::System::GetNow()),
                   static_cast<unsigned long>(seed.AudioSampleRate()),
                   static_cast<unsigned long>(seed.AudioBlockSize()),
                   static_cast<unsigned long>(delta));
#endif
  }
}
