#pragma once
#include <string>
#include <vector>
#include <atomic>

struct Points
{
  std::size_t point;
  const char *msg;
};

struct Note
{
  int x;
  int y;
  long long timestamp;
};

struct Song
{
  std::string name;
  std::vector<Note> notes;
};

struct Score
{
  std::size_t note_no = 0;
  std::size_t points = 0;
  std::size_t combo = 0;
  std::size_t max_combo = 0;
};
