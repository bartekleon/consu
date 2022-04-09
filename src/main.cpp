#define NOMINMAX

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/canvas.hpp>
#include <fmt/format.h>
#include <fmt/printf.h>

#include <chrono>
#include <thread>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "audio.hpp"
#include "constants.hpp"
#include "types.hpp"
#include "utils.hpp"


class Game
{
  enum class GameState {
    MainMenu = 0,
    SongsMenu,
    Play = 10,
    CreateSongMetadata = 20,
    CreateSongMetadata2,
    CreateSongData,
    SaveSongData
  };

  static constexpr auto TIMING = std::chrono::milliseconds(1000 / 120);
  ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::TerminalOutput();

  GameState state = GameState::MainMenu;
  Song song;
  std::size_t note_no = 0;
  std::chrono::steady_clock::time_point timepoint;
  int mouse_x = 0;
  int mouse_y = 0;
  std::chrono::steady_clock::time_point clickpoint;
  std::size_t points = 0;
  Audio audio_player;
  std::string last_hit;

  ftxui::Component inputs = ftxui::Container::Vertical({});
  ftxui::Component render = ftxui::Container::Vertical({});

public:
  std::function<void()> load_song(const std::string& s)
  {
    return [&, s] {
      const std::string path = fmt::format("{}/{}/{}.{}", FILE_CONSTANTS::FOLDER_PATH, s, s, FILE_CONSTANTS::GAMEFILE_EXT);
      std::ifstream file{ path };

      song = {};
      std::string line;
      std::getline(file, line);
      song.name = line;

      while (std::getline(file, line)) {
        if (line.empty()) { break; }
        std::istringstream iss(line);
        int x{};
        int y{};
        long long stamp{};
        if (!(iss >> x >> y >> stamp)) { break; }// error

        song.notes.push_back({ x, y, stamp });
        // process pair (a,b)
      }

      note_no = 0;
      points = 0;
      mouse_x = -1;
      mouse_y = -1;
      timepoint = std::chrono::steady_clock::now();
      game_iteration(GameState::Play);
    };
  }
  auto initialize_song()
  {
    const std::filesystem::path sandbox{ "songs/" + song.name };
    std::filesystem::path p = "";
    for (auto const &dir_entry : std::filesystem::directory_iterator{ sandbox }) {
      if (dir_entry.is_regular_file() && dir_entry.path().extension() != ".consu") { p = dir_entry.path(); }
    }
    if (p.empty()) {
      return;
    }
    audio_player.init_song(p.string().c_str());
  }

  ftxui::Component main_menu()
  {
    inputs->DetachAllChildren();
    inputs->Add(ftxui::Button("Songs menu", [&] { game_iteration(GameState::SongsMenu); }));
    inputs->Add(ftxui::Button("Create song", [&] { game_iteration(GameState::CreateSongMetadata); }));
    return ftxui::Renderer([&] { return ftxui::hbox({ ftxui::text("main menu"), inputs->Render() }); });
  }
  
  ftxui::Component songs_menu()
  {
    const std::filesystem::path sandbox{ "songs" };

    inputs->DetachAllChildren();

    for (auto const &dir_entry : std::filesystem::directory_iterator{ sandbox }) {
      if (dir_entry.is_directory()) {
        inputs->Add(
          ftxui::Button(dir_entry.path().stem().string(), load_song(dir_entry.path().stem().string())));
      }
    }

    return ftxui::Renderer([&] { return ftxui::hbox({ ftxui::text("songs_menu"), inputs->Render() }); });
  }
  ftxui::Component play_game()
  {
    inputs->DetachAllChildren();
    initialize_song();

    auto container = ftxui::Container::Horizontal({});
    auto event_handler = ftxui::CatchEvent(container, [&](ftxui::Event ev) {
      if (ev.mouse().button == ftxui::Mouse::Left && ev.mouse().motion == ftxui::Mouse::Pressed) {
        if (note_no + 1 != song.notes.size()) {
          const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
          const auto &note = song.notes.at(note_no);
          if (std::hypot(note.x - ev.mouse().x * CANVAS_CONSTANTS::RATIO_FIX[0], note.y - ev.mouse().y * CANVAS_CONSTANTS::RATIO_FIX[1]) < CANVAS_CONSTANTS::CURATED_DISTANCE) {
            const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now - timepoint).count();
            const long long time_to_click = note.timestamp - timestamp;
            const auto [point, msg] = time_to_points(time_to_click);
            points += point;
            last_hit = msg;
            ++note_no;
          }
        }
      }
      return false;
    });

    inputs->Add(event_handler);

    return ftxui::Renderer([&] {
      ftxui::Canvas c = ftxui::Canvas(CANVAS_CONSTANTS::SIZE, CANVAS_CONSTANTS::SIZE);
      const std::size_t last_note = std::min(note_no + CANVAS_CONSTANTS::NOTES_ON_SCREEN_LIMIT, song.notes.size());
      const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
      const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now - timepoint).count();
      for (std::size_t n = note_no; n < last_note; ++n) {
        long long time_to_click = song.notes.at(n).timestamp - timestamp;
        if (time_to_click > TIME_CONSTANTS::MAX_TIME_DISPLAY_NOTE) { break; }
        if (time_to_click < 0) {
          if (note_no + 1 != song.notes.size()) {
            last_hit = "miss";
            ++note_no;
          }
        } else {
          c.DrawPointCircle(song.notes.at(n).x, song.notes.at(n).y, CANVAS_CONSTANTS::CIRCLE_DIAMETER, time_to_color(time_to_click));
        }
      }

      const std::string res = "points: " + std::to_string(points) + " / " + last_hit;
      return ftxui::hbox({ ftxui::canvas(std::move(c)) | ftxui::border, ftxui::text(res) });
    });
  }

  ftxui::Component create_song_metadata()
  {
    song = Song{};

    inputs->DetachAllChildren();
    inputs->Add(ftxui::Input(&song.name, "song name"));
    inputs->Add(ftxui::Button("Create song", [&] { game_iteration(GameState::CreateSongMetadata2); }));

    return ftxui::Renderer([&] { return ftxui::hbox({ ftxui::text("metadata"), inputs->Render() }); });
  }
  ftxui::Component create_song_metadata2()
  {
    std::filesystem::create_directories("songs/" + song.name);

    inputs->DetachAllChildren();
    inputs->Add(ftxui::Button("Create song", [&] { game_iteration(GameState::CreateSongData); }));

    return ftxui::Renderer([&] {
      return ftxui::hbox(
        { ftxui::text("folder has been created in: songs/" + song.name + ", add music file there and press button"),
          inputs->Render() });
    });
  }
  ftxui::Component create_song()
  {
    initialize_song();
    timepoint = std::chrono::steady_clock::now();
    inputs->DetachAllChildren();

    auto container = ftxui::Container::Vertical({});
    auto event_handler = ftxui::CatchEvent(container, [&](ftxui::Event ev) {
      if (audio_player.has_ended()) { game_iteration(GameState::SaveSongData); }
      if (ev.mouse().button == ftxui::Mouse::Left && ev.mouse().motion == ftxui::Mouse::Pressed) {
        song.notes.push_back({ ev.mouse().x,
          ev.mouse().y,
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timepoint)
            .count() });
      } else if (ev == ftxui::Event::Escape) {
        audio_player.stop();
        game_iteration(GameState::SaveSongData);
      }
      return false;
    });

    inputs->Add(event_handler);

    return ftxui::Renderer([&] {
      ftxui::Canvas c = ftxui::Canvas(CANVAS_CONSTANTS::SIZE, CANVAS_CONSTANTS::SIZE);
      return ftxui::hbox({ ftxui::canvas(std::move(c)) | ftxui::border });
    });    
  }
  ftxui::Component save_song()
  {
    inputs->DetachAllChildren();
    std::ofstream file{ "songs/" + song.name + "/" + song.name + ".consu" };

    file << song.name << '\n';
    for (const auto &note : song.notes) { file << note.x << ' ' << note.y << ' ' << note.timestamp << '\n'; }

    inputs->Add(ftxui::Button("Back to menu", [&] { game_iteration(GameState::MainMenu); }));

    return ftxui::Renderer(inputs, [&] { return ftxui::hbox({ ftxui::text("file saved"), inputs->Render() }); });
  }

  ftxui::Component renderer()
  {
    switch (state) {
    case GameState::MainMenu:
      return main_menu();
    case GameState::SongsMenu:
      return songs_menu();
    case GameState::Play:
      return play_game();
    case GameState::CreateSongMetadata:
      return create_song_metadata();
    case GameState::CreateSongMetadata2:
      return create_song_metadata2();
    case GameState::CreateSongData:
      return create_song();
    case GameState::SaveSongData:
      return save_song();
    }
    return ftxui::Container::Vertical({});
  }

  void game_iteration(GameState new_state)
  {
    if (state != new_state) {
      state = new_state;
      render = renderer();
    }
  }

  Game() : render{ renderer() }
  {
    auto renderer = ftxui::Renderer(inputs, [&] { return render->Render(); });
    std::atomic<bool> refresh_ui_continue = true;

    std::thread refresh_ui([&] {
      while (refresh_ui_continue) {
        std::this_thread::sleep_for(TIMING);
        screen.PostEvent(ftxui::Event::Custom);
      }
    });
    screen.Loop(renderer);
    refresh_ui_continue = false;
    refresh_ui.join();
  }
};

int main()
{
  try {
    Game game;
  } catch (const std::exception &e) {
    fmt::print("Unhandled exception in main: {}", e.what());
  }

  return 0;
}