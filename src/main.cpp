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
#include <string>

#include "constants.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "audio.hpp"
#include "song.hpp"

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
  std::size_t points = 0;
  std::string last_hit;

  ftxui::Component inputs = ftxui::Container::Vertical({});
  ftxui::Component render = ftxui::Container::Vertical({});
  Audio audio_player;
  
private:
  std::function<void()> load_song(const std::string& s)
  {
    return [&, s] {
      song = load_song_from_disk(s);
      note_no = 0;
      points = 0;
      timepoint = std::chrono::steady_clock::now();
      game_iteration(GameState::Play);
    };
  }
  void initialize_song()
  {
    if (const std::string p = find_song(song); !p.empty()) {
      audio_player.init_song(fmt::format("{}/{}/{}", FILE_CONSTANTS::FOLDER_PATH, song.name, p));
    }
  }

  ftxui::Component main_menu()
  {
    inputs->Add(ftxui::Button(std::string("Songs menu"), [&] { game_iteration(GameState::SongsMenu); }));
    inputs->Add(ftxui::Button(std::string("Create song"), [&] { game_iteration(GameState::CreateSongMetadata); }));

    return ftxui::Renderer([&] {
      return ftxui::vbox({
        ftxui::hbox({
          ftxui::filler(),
          ftxui::vbox({
            ftxui::text(R"(  _____ ____  _   _  _____ _    _)"),
            ftxui::text(R"( / ____/ __ \| \ | |/ ____| |  | |)"),
            ftxui::text(R"(| |   | |  | |  \| | (___ | |  | |)"),
            ftxui::text(R"(| |   | |  | | . ` |\___ \| |  | |)"),
            ftxui::text(R"(| |___| |__| | |\  |____) | |__| |)"),
            ftxui::text(R"( \_____\____/|_| \_|_____/ \____/)")
          }),
          ftxui::filler()
        }),
        ftxui::text(" "),
        ftxui::hbox({
          ftxui::filler(),
          ftxui::vbox({ inputs->Render() }),
          ftxui::filler()
        })
      });
    });
  }
  
  ftxui::Component songs_menu()
  {
    const std::filesystem::path path{ FILE_CONSTANTS::FOLDER_PATH };
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }
    for (auto const &dir_entry : std::filesystem::directory_iterator{ path }) {
      if (dir_entry.is_directory()) {
        const auto stem = dir_entry.path().stem().string();
        inputs->Add(ftxui::Button(stem, load_song(stem)));
      }
    }

    return ftxui::Renderer([&] {
      return ftxui::hbox({ ftxui::text("songs_menu"),
        inputs->Render() | ftxui::vscroll_indicator | ftxui::frame | ftxui::border });
    });
  }
  ftxui::Component play_game()
  {
    initialize_song();

    auto container = ftxui::Container::Horizontal({});
    auto event_handler = ftxui::CatchEvent(container, [&](ftxui::Event ev) {
      if (ev.mouse().button == ftxui::Mouse::Left && ev.mouse().motion == ftxui::Mouse::Pressed) {
        if (note_no + 1 != song.notes.size()) {
          const auto now = std::chrono::steady_clock::now();
          const auto &note = song.notes.at(note_no);
          if (std::hypot(note.x - ev.mouse().x * CANVAS_CONSTANTS::RATIO_FIX[0], note.y - ev.mouse().y * CANVAS_CONSTANTS::RATIO_FIX[1]) < CANVAS_CONSTANTS::CURATED_DISTANCE) {
            const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now - timepoint).count();
            const auto time_to_click = note.timestamp - timestamp;
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
      const auto last_note = std::min(note_no + CANVAS_CONSTANTS::NOTES_ON_SCREEN_LIMIT, song.notes.size());
      const auto now = std::chrono::steady_clock::now();
      const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now - timepoint).count();
      for (auto n = note_no; n < last_note; ++n) {
        const auto time_to_click = song.notes.at(n).timestamp - timestamp;
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

      const std::string res = fmt::format("points: {} / {}", points, last_hit);
      return ftxui::hbox({ ftxui::canvas(std::move(c)) | ftxui::border, ftxui::text(res) });
    });
  }

  ftxui::Component create_song_metadata()
  {
    song = Song{};

    inputs->Add(ftxui::Input(&song.name, "song name"));
    inputs->Add(ftxui::Button(std::string("Create song"), [&] { game_iteration(GameState::CreateSongMetadata2); }));

    return ftxui::Renderer([&] { return ftxui::hbox({ ftxui::text("metadata"), inputs->Render() }); });
  }
  ftxui::Component create_song_metadata2()
  {
    std::filesystem::create_directories(generate_folder_path(song.name));

    inputs->Add(ftxui::Button(std::string("Create song"), [&] { game_iteration(GameState::CreateSongData); }));

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
    save_song_to_disk(song);

    inputs->Add(ftxui::Button(std::string("Back to menu"), [&] { game_iteration(GameState::MainMenu); }));

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
      inputs->DetachAllChildren();
      render = renderer();
    }
  }

public:
  Game() : render{ renderer() }
  {
    auto r = ftxui::Renderer(inputs, [&] { return render->Render(); });
    std::atomic<bool> refresh_ui_continue = true;

    std::thread refresh_ui([&] {
      while (refresh_ui_continue) {
        std::this_thread::sleep_for(TIMING);
        screen.PostEvent(ftxui::Event::Custom);
      }
    });
    screen.Loop(r);
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
