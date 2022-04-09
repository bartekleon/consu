#pragma once

#include <ftxui/screen/color.hpp>

#include "constants.hpp"


static constexpr ftxui::Color::Palette16 time_to_color(const long long time_ms) noexcept
{
  if (time_ms > TIME_CONSTANTS::DISPLAY_NOTE_COLOR_3) { return ftxui::Color::Blue; }
  if (time_ms > TIME_CONSTANTS::DISPLAY_NOTE_COLOR_2) { return ftxui::Color::Green; }
  if (time_ms > TIME_CONSTANTS::DISPLAY_NOTE_COLOR_1) { return ftxui::Color::Yellow; }
  return ftxui::Color::Red;
}

static Points time_to_points(const long long time_ms) noexcept
{
  if (time_ms > TIME_CONSTANTS::NOTE_MISS) return POINT_CONSTANTS::MISS;
  if (time_ms > TIME_CONSTANTS::NOTE_OK) return POINT_CONSTANTS::OK;
  if (time_ms > TIME_CONSTANTS::NOTE_NICE) return POINT_CONSTANTS::NICE;
  if (time_ms > TIME_CONSTANTS::NOTE_GOOD) return POINT_CONSTANTS::GOOD;
  return POINT_CONSTANTS::PERFECT;
}
