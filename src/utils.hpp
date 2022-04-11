#pragma once

#include <ftxui/screen/color.hpp>
#include <fmt/format.h>

#include "constants.hpp"


static constexpr ftxui::Color::Palette256 time_to_color(const long long time_ms) noexcept
{
  if (time_ms > TIME_CONSTANTS::MAX_TIME_DISPLAY_NOTE) { return ftxui::Color::Blue1; }
  if (time_ms > TIME_CONSTANTS::TIME_PREP) { return ftxui::Color::Green1; }
  if (time_ms > TIME_CONSTANTS::NOTE_MISS) { return ftxui::Color::Yellow1; }
  if (time_ms > TIME_CONSTANTS::NOTE_OK) { return ftxui::Color::DarkOrange; }
  if (time_ms > TIME_CONSTANTS::NOTE_NICE) { return ftxui::Color::OrangeRed1; }
  if (time_ms > TIME_CONSTANTS::NOTE_GOOD) { return ftxui::Color::Red3; }
  return ftxui::Color::Red1;
}

static Points time_to_points(const long long time_ms) noexcept
{
  if (time_ms > TIME_CONSTANTS::NOTE_MISS) return POINT_CONSTANTS::MISS;
  if (time_ms > TIME_CONSTANTS::NOTE_OK) return POINT_CONSTANTS::OK;
  if (time_ms > TIME_CONSTANTS::NOTE_NICE) return POINT_CONSTANTS::NICE;
  if (time_ms > TIME_CONSTANTS::NOTE_GOOD) return POINT_CONSTANTS::GOOD;
  return POINT_CONSTANTS::PERFECT;
}

static std::string generate_folder_path(const std::string &file_name)
{
  return fmt::format("{}/{}", FILE_CONSTANTS::FOLDER_PATH, file_name);
}
static std::string generate_map_path(const std::string& file_name)
{
  return fmt::format("{}/{}/{}{}", FILE_CONSTANTS::FOLDER_PATH, file_name, file_name, FILE_CONSTANTS::GAMEFILE_EXT);
}
