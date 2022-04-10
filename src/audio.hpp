#pragma once

#include <smk/SoundBuffer.hpp>
#include <smk/Sound.hpp>
#include <smk/Audio.hpp>
#include <string>

class Audio
{
  // inout - initialized during "init_song"
  smk::Sound sound;// NOLINT
  smk::SoundBuffer buffer;
  smk::Audio audio;

public:
  void init_song(const std::string& path)
  {
    buffer = smk::SoundBuffer(path);
    sound = smk::Sound(buffer);
    sound.Play();
  }
  bool has_ended() { return !sound.IsPlaying(); }
  void stop() { sound.Stop(); }
};
