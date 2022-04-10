#pragma once

#include <fstream>
#include <sstream>
#include <filesystem>

#include "types.hpp"
#include "utils.hpp"
#include "constants.hpp"

void save_song_to_disk(const Song& song)
{
  std::ofstream file{ generate_map_path(song.name) };

  file << song.name << '\n';
  for (const auto &note : song.notes) { file << note.x << ' ' << note.y << ' ' << note.timestamp << '\n'; }
}

Song load_song_from_disk(const std::string &file_name)
{
  std::ifstream file{ generate_map_path(file_name) };

  Song song{};
  std::string line;
  std::getline(file, line);
  song.name = line;

  while (std::getline(file, line)) {
    if (line.empty()) { break; }
    std::istringstream iss(line);
    int x{};
    int y{};
    long long stamp{};
    if (!(iss >> x >> y >> stamp)) { break; }

    song.notes.push_back({ x, y, stamp });
  }

  return song;
}


std::string find_song(const Song& song)
{
  for (auto const &dir_entry : std::filesystem::directory_iterator{ generate_folder_path(song.name) }) {
    if (dir_entry.is_regular_file() && dir_entry.path().extension() != FILE_CONSTANTS::GAMEFILE_EXT) {
      return dir_entry.path().filename().string();
    }
  }
  return std::string{};
}