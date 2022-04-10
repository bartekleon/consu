#pragma once
#include <array>

#include "types.hpp"

struct TIME_CONSTANTS
{
  static constexpr long long MAX_TIME_DISPLAY_NOTE = 1000;
  static constexpr long long DISPLAY_NOTE_COLOR_3 = MAX_TIME_DISPLAY_NOTE;
  static constexpr long long DISPLAY_NOTE_COLOR_2 = 700;
  static constexpr long long DISPLAY_NOTE_COLOR_1 = 400;
  static constexpr long long NOTE_MISS = 400;
  static constexpr long long NOTE_OK = 300;
  static constexpr long long NOTE_NICE = 200;
  static constexpr long long NOTE_GOOD = 100;
};

struct POINT_CONSTANTS
{
  static constexpr Points MISS = { 0, "miss" };
  static constexpr Points OK = { 100, "ok" };
  static constexpr Points NICE = { 200, "nice" };
  static constexpr Points GOOD = { 300, "good!" };
  static constexpr Points PERFECT = { 500, "PERFECT!" };
};

struct CANVAS_CONSTANTS
{
  static constexpr std::size_t SIZE = 150;
  // this should be a little bit bigger than circle size due to console inaccuracy
  static constexpr double CURATED_DISTANCE = 8;
  static constexpr int CIRCLE_DIAMETER = 5;
  // The smallest unit in a canvas are the braille characters. You can fit 2x4 braille characters in a single cell.
  // in case of DrawBlockCircle 2x2 should be used
  static constexpr std::array<int, 2> RATIO_FIX = { 2, 4 };
  static constexpr std::size_t NOTES_ON_SCREEN_LIMIT = 5;
};

struct FILE_CONSTANTS
{
  static constexpr char FOLDER_PATH[] = "songs";
  static constexpr char GAMEFILE_EXT[] = ".consu";
};
