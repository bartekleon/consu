#pragma once
#include <string>
#include <vector>

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
