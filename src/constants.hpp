#pragma once
#include <array>

#include "types.hpp"

struct TIME_CONSTANTS
{
  static constexpr long long MAX_TIME_DISPLAY_NOTE = 1000;
  static constexpr long long TIME_PREP = 700;
  static constexpr long long NOTE_MISS = 400;
  static constexpr long long NOTE_OK = 300;
  static constexpr long long NOTE_NICE = 200;
  static constexpr long long NOTE_GOOD = 100;
};

struct POINT_CONSTANTS
{
  static constexpr char MISS_TXT[] = "miss";
  static constexpr char OK_TXT[] = "ok";
  static constexpr char NICE_TXT[] = "nice";
  static constexpr char GOOD_TXT[] = "good!";
  static constexpr char PERFECT_TXT[] = "PERFECT!";

  static constexpr Points MISS = { 0, MISS_TXT };
  static constexpr Points OK = { 100, OK_TXT };
  static constexpr Points NICE = { 200, NICE_TXT };
  static constexpr Points GOOD = { 300, GOOD_TXT };
  static constexpr Points PERFECT = { 500, PERFECT_TXT };
};

struct CANVAS_CONSTANTS
{
  static constexpr std::size_t SIZE = 150;
  static constexpr int CIRCLE_DIAMETER = 10;
  static constexpr std::size_t NOTES_ON_SCREEN_LIMIT = 6;
  static constexpr std::array<std::size_t, 2> RATIO_FIX{ 2, 4 };
};

struct FILE_CONSTANTS
{
  static constexpr char FOLDER_PATH[] = "songs";
  static constexpr char GAMEFILE_EXT[] = ".consu";
};
