#pragma once

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

class Audio
{
  // inout - initialized in constructor
  ma_engine engine;// NOLINT
  // inout - initialized during "init_song"
  ma_sound sound;// NOLINT

public:
  Audio() { ma_engine_init(nullptr, &engine); }// NOLINT
  ~Audio() { ma_engine_uninit(&engine); }
  Audio(const Audio &) = delete;
  Audio(Audio &&) = delete;
  Audio operator=(const Audio &) = delete;
  Audio operator=(Audio &&) = delete;

  void init_song(const char *path)
  {
    ma_sound_config soundConfig = ma_sound_config_init();
    soundConfig.pFilePath = path;// Set this to load from a file path.
    soundConfig.pDataSource = nullptr;// Set this to initialize from an existing data source.
    soundConfig.pInitialAttachment = nullptr;
    soundConfig.initialAttachmentInputBusIndex = 0;
    soundConfig.channelsIn = 1;
    soundConfig.channelsOut = 0;// Set to 0 to use the engine's native channel count.

    ma_sound_init_ex(&engine, &soundConfig, &sound);
    ma_sound_start(&sound);
  }
  bool has_ended() { return ma_sound_at_end(&sound) != MA_FALSE; }
  void stop() { ma_engine_stop(&engine); }
};