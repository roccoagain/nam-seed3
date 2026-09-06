#include "daisy_seed.h"
#include "nam_audio.h"
#include <atomic>
#include <cstdint>

daisy::DaisySeed seed;
static NamAudio audio;
static std::atomic<uint32_t> audio_callbacks{0};
static std::atomic<uint32_t> max_callback_us{0};
static std::atomic<uint32_t> overruns{0};
static std::atomic<uint32_t> requested_amp{1};
static_assert(ATOMIC_INT_LOCK_FREE == 2, "Audio counters must be lock-free");
// Only changed while audio is stopped.
static bool bypass = false;

void AudioCallback(daisy::AudioHandle::InputBuffer in,
                   daisy::AudioHandle::OutputBuffer out, size_t size) {
  const uint32_t start = daisy::System::GetUs();
  audio.Process(in[0], out[0], out[1], size, bypass);
  const uint32_t elapsed = daisy::System::GetUs() - start;
  uint32_t previous = max_callback_us.load(std::memory_order_relaxed);
  while (elapsed > previous &&
         !max_callback_us.compare_exchange_weak(previous, elapsed,
                                                std::memory_order_relaxed)) {
  }
  if (elapsed >= 1000)
    overruns.fetch_add(1, std::memory_order_relaxed);
  audio_callbacks.fetch_add(1, std::memory_order_relaxed);
}

// USB interrupt: record a command only. Never allocate or construct a model
// here.
void SerialReceive(uint8_t *data, uint32_t *length) {
  for (uint32_t i = 0; i < *length; ++i)
    if (data[i] >= '0' && data[i] <= '3')
      requested_amp.store(data[i] - '0', std::memory_order_relaxed);
}

int main() {
  seed.Init();
  seed.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);
  seed.SetAudioBlockSize(NamAudio::kBlockSize);
  seed.StartLog(false);
  seed.usb_handle.SetReceiveCallback(SerialReceive,
                                     daisy::UsbHandle::FS_INTERNAL);
  AmpId selected = AmpId::Fender;
  uint32_t current_command = 1;
  bool model_ready = audio.Init(selected);
  seed.StartAudio(AudioCallback);
  uint32_t last_report = daisy::System::GetNow();
  bool led_on = false;
  while (true) {
    const uint32_t command = requested_amp.load(std::memory_order_relaxed);
    if (command != current_command) {
      // Stop DMA before changing the model or its state. Construction and
      // prewarming may allocate and take longer than an audio block.
      seed.StopAudio();
      bypass = command == 0;
      if (!bypass) {
        selected = static_cast<AmpId>(command);
        model_ready = audio.Init(selected);
      }
      current_command = command;
      seed.StartAudio(AudioCallback);
    }
    const uint32_t now = daisy::System::GetNow();
    if (now - last_report >= 1000) {
      last_report = now;
      led_on = !led_on;
      seed.SetLed(led_on);
      seed.PrintLine(
          "NAM A2-Lite: amp=%s ready=%u bypass=%u callbacks=%lu "
          "max_us=%lu overruns=%lu | 0=clean 1=Twin65 2=AC30 3=JCM800",
          AmpName(selected), static_cast<unsigned>(model_ready),
          static_cast<unsigned>(bypass || !model_ready),
          static_cast<unsigned long>(audio_callbacks.exchange(0)),
          static_cast<unsigned long>(max_callback_us.exchange(0)),
          static_cast<unsigned long>(overruns.exchange(0)));
    }
    seed.DelayMs(1);
  }
}
