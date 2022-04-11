#define NOMINMAX

#include <fmt/format.h>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/canvas.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

#include "audio.hpp"
#include "constants.hpp"
#include "song.hpp"
#include "types.hpp"
#include "utils.hpp"

class Game
{
  enum class GameState {
    MainMenu = 0,
    SongsMenu,
    Play = 10,
    Scoreboard,
    CreateSongMetadata = 20,
    CreateSongMetadata2,
    CreateSongData,
    SaveSongData
  };

  static constexpr auto TIMING = std::chrono::milliseconds(1000 / 120);
  ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::TerminalOutput();

  GameState state = GameState::MainMenu;
  Song song;
  Score score{};
  std::chrono::steady_clock::time_point timepoint;
  std::string last_hit;

  ftxui::Component inputs = ftxui::Container::Vertical({});
  ftxui::Component render = ftxui::Container::Vertical({});
  Audio audio_player;

private:
  std::function<void()> load_song(const std::string &s)
  {
    return [&, s] {
      song = load_song_from_disk(s);
      score = {};
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
      return ftxui::vbox({ ftxui::hbox({ ftxui::filler(),
                             ftxui::vbox({ ftxui::text(R"(  _____ ____  _   _  _____ _    _)"),
                               ftxui::text(R"( / ____/ __ \| \ | |/ ____| |  | |)"),
                               ftxui::text(R"(| |   | |  | |  \| | (___ | |  | |)"),
                               ftxui::text(R"(| |   | |  | | . ` |\___ \| |  | |)"),
                               ftxui::text(R"(| |___| |__| | |\  |____) | |__| |)"),
                               ftxui::text(R"( \_____\____/|_| \_|_____/ \____/)") }),
                             ftxui::filler() }),
        ftxui::text(" "),
        ftxui::hbox({ ftxui::filler(), ftxui::vbox({ inputs->Render() }), ftxui::filler() }) });
    });
  }

  ftxui::Component songs_menu()
  {
    auto container = ftxui::Renderer([] { return ftxui::text(""); });
    auto event_handler = ftxui::CatchEvent(container, [&](ftxui::Event ev) {
      if (ev == ftxui::Event::Escape) { game_iteration(GameState::MainMenu); }
      return false;
    });

    inputs->Add(event_handler);

    const auto stems = get_songs_list();
    for (const auto &stem : stems) { inputs->Add(ftxui::Button(stem, load_song(stem))); }

    return ftxui::Renderer([&] {
      return ftxui::vbox({ ftxui::hbox({ ftxui::filler(),
                             ftxui::vbox({
                               ftxui::text(R"(  _____                                                    )"),
                               ftxui::text(R"( / ____|                                                   )"),
                               ftxui::text(R"(| (___   ___  _ __   __ _ ___   _ __ ___   ___ _ __  _   _ )"),
                               ftxui::text(R"( \___ \ / _ \| '_ \ / _` / __| | '_ ` _ \ / _ \ '_ \| | | |)"),
                               ftxui::text(R"( ____) | (_) | | | | (_| \__ \ | | | | | |  __/ | | | |_| |)"),
                               ftxui::text(R"(|_____/ \___/|_| |_|\__, |___/ |_| |_| |_|\___|_| |_|\__,_|)"),
                               ftxui::text(R"(                     __/ |                                 )"),
                               ftxui::text(R"(                    |___/                                  )"),
                             }),
                             ftxui::filler() }),
        ftxui::text(" "),
        ftxui::hbox({ ftxui::filler(),
          ftxui::vbox(
            { inputs->Render() | ftxui::vscroll_indicator | ftxui::frame | ftxui::border
              | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 60) | ftxui::size(ftxui::HEIGHT, ftxui::GREATER_THAN, 20) }),
          ftxui::filler() }) });
    });
  }

  ftxui::Component play_game()
  {
    initialize_song();

    auto container = ftxui::Renderer([] { return ftxui::text(""); });
    auto event_handler = ftxui::CatchEvent(container, [&](ftxui::Event ev) {
      if (ev.mouse().button == ftxui::Mouse::Left && ev.mouse().motion == ftxui::Mouse::Pressed) {
        if (score.note_no + 1 != song.notes.size()) {
          const auto now = std::chrono::steady_clock::now();
          const auto &note = song.notes.at(score.note_no);
          if (std::hypot(note.x - ev.mouse().x, note.y - ev.mouse().y) <= CANVAS_CONSTANTS::CIRCLE_DIAMETER) {
            const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now - timepoint).count();
            const auto time_to_click = note.timestamp - timestamp;
            const auto [point, msg] = time_to_points(time_to_click);
            if (point == POINT_CONSTANTS::MISS.point) {
              score.combo = 0;
            } else {
              ++score.combo;
              score.max_combo = std::max(score.combo, score.max_combo);
              score.points += point;
            }
            last_hit = msg;
            ++score.note_no;
          }
        }
      }
      return false;
    });

    inputs->Add(event_handler);

    return ftxui::Renderer([&] {
      if (audio_player.has_ended()) { game_iteration(GameState::Scoreboard); }
      ftxui::Canvas c = ftxui::Canvas(CANVAS_CONSTANTS::SIZE, CANVAS_CONSTANTS::SIZE);
      const auto last_note = std::min(score.note_no + CANVAS_CONSTANTS::NOTES_ON_SCREEN_LIMIT, song.notes.size());
      const auto now = std::chrono::steady_clock::now();
      const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now - timepoint).count();
      for (auto n = score.note_no; n < last_note; ++n) {
        const auto time_to_click = song.notes.at(n).timestamp - timestamp;
        if (time_to_click > TIME_CONSTANTS::MAX_TIME_DISPLAY_NOTE) { break; }
        if (time_to_click < 0) {
          if (score.note_no + 1 != song.notes.size()) {
            last_hit = POINT_CONSTANTS::MISS_TXT;
            ++score.note_no;
            score.combo = 0;
          }
        } else {
          c.DrawPointCircle(song.notes.at(n).x * CANVAS_CONSTANTS::RATIO_FIX[0],
            song.notes.at(n).y * CANVAS_CONSTANTS::RATIO_FIX[1],
            CANVAS_CONSTANTS::CIRCLE_DIAMETER,
            time_to_color(time_to_click));
        }
      }

      return ftxui::hbox({ ftxui::canvas(std::move(c)) | ftxui::border,
        ftxui::vbox({
          ftxui::text(fmt::format("SONG NAME: {}", song.name)),
          ftxui::text(" "),
          ftxui::text(fmt::format("POINTS: {}", score.points)),
          ftxui::text(" "),
          ftxui::text(fmt::format("LAST HIT: {}", last_hit)),
          ftxui::text(" "),
          ftxui::text(fmt::format("CURRENT COMBO: {}", score.combo)),
          ftxui::text(" "),
          ftxui::text(fmt::format("MAX COMBO: {}", score.max_combo)),
        }) });
    });
  }

  ftxui::Component display_end_score()
  {
    inputs->Add(ftxui::Button(std::string("Back to menu"), [&] { game_iteration(GameState::MainMenu); }));

    return ftxui::Renderer([&] {
      return ftxui::vbox(
        { ftxui::hbox({ ftxui::filler(),
            ftxui::vbox({
              ftxui::text(R"(   _____                            _         _       _   _                 )"),
              ftxui::text(R"(  / ____|                          | |       | |     | | (_)                )"),
              ftxui::text(R"( | |     ___  _ __   __ _ _ __ __ _| |_ _   _| | __ _| |_ _  ___  _ __  ___ )"),
              ftxui::text(R"( | |    / _ \| '_ \ / _` | '__/ _` | __| | | | |/ _` | __| |/ _ \| '_ \/ __|)"),
              ftxui::text(R"( | |___| (_) | | | | (_| | | | (_| | |_| |_| | | (_| | |_| | (_) | | | \__ \)"),
              ftxui::text(R"(  \_____\___/|_| |_|\__, |_|  \__,_|\__|\__,_|_|\__,_|\__|_|\___/|_| |_|___/)"),
              ftxui::text(R"(                     __/ |                                                  )"),
              ftxui::text(R"(                    |___/                                                   )"),
            }),
            ftxui::filler() }),
          ftxui::vbox({ ftxui::text(fmt::format("SONG NAME: {}", song.name)),
            ftxui::text(" "),
            ftxui::text(fmt::format("POINTS: {}", score.points)),
            ftxui::text(" "),
            ftxui::text(fmt::format("LAST HIT: {}", last_hit)),
            ftxui::text(" "),
            ftxui::text(fmt::format("CURRENT COMBO: {}", score.combo)),
            ftxui::text(" "),
            ftxui::text(fmt::format("MAX COMBO: {}", score.max_combo)),
            ftxui::text(" "),
            inputs->Render() | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 60) }) });
    });
  }

  ftxui::Component create_song_metadata()
  {
    song = Song{};

    inputs->Add(ftxui::Input(&song.name, "song name"));
    inputs->Add(ftxui::Renderer([] { return ftxui::text(" "); }));
    inputs->Add(ftxui::Button(std::string("Create song"), [&] { game_iteration(GameState::CreateSongMetadata2); }));
    inputs->Add(ftxui::Renderer([] { return ftxui::text(" "); }));
    inputs->Add(ftxui::Button(std::string("Back to menu"), [&] { game_iteration(GameState::MainMenu); }));

    return ftxui::Renderer([&] {
      return ftxui::vbox({ ftxui::hbox({ ftxui::filler(),
                             ftxui::vbox({
                               ftxui::text(R"(             _     _                                               )"),
                               ftxui::text(R"(    /\      | |   | |                                              )"),
                               ftxui::text(R"(   /  \   __| | __| |  _ __   _____      __  ___  ___  _ __   __ _ )"),
                               ftxui::text(R"(  / /\ \ / _` |/ _` | | '_ \ / _ \ \ /\ / / / __|/ _ \| '_ \ / _` |)"),
                               ftxui::text(R"( / ____ \ (_| | (_| | | | | |  __/\ V  V /  \__ \ (_) | | | | (_| |)"),
                               ftxui::text(R"(/_/    \_\__,_|\__,_| |_| |_|\___| \_/\_/   |___/\___/|_| |_|\__, |)"),
                               ftxui::text(R"(                                                              __/ |)"),
                               ftxui::text(R"(                                                             |___/ )"),
                             }),
                             ftxui::filler() }),
        ftxui::text(" "),
        ftxui::hbox({ ftxui::filler(),
          ftxui::vbox({ inputs->Render() | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 60) }),
          ftxui::filler() }) });
    });
  }
  ftxui::Component create_song_metadata2()
  {
    std::filesystem::create_directories(generate_folder_path(song.name));

    inputs->Add(ftxui::Button(std::string("Create song"), [&] { game_iteration(GameState::CreateSongData); }));
    return ftxui::Renderer([&] {
      return ftxui::vbox({ ftxui::hbox({ ftxui::filler(),
                             ftxui::vbox({
                               ftxui::text(R"(             _     _                                               )"),
                               ftxui::text(R"(    /\      | |   | |                                              )"),
                               ftxui::text(R"(   /  \   __| | __| |  _ __   _____      __  ___  ___  _ __   __ _ )"),
                               ftxui::text(R"(  / /\ \ / _` |/ _` | | '_ \ / _ \ \ /\ / / / __|/ _ \| '_ \ / _` |)"),
                               ftxui::text(R"( / ____ \ (_| | (_| | | | | |  __/\ V  V /  \__ \ (_) | | | | (_| |)"),
                               ftxui::text(R"(/_/    \_\__,_|\__,_| |_| |_|\___| \_/\_/   |___/\___/|_| |_|\__, |)"),
                               ftxui::text(R"(                                                              __/ |)"),
                               ftxui::text(R"(                                                             |___/ )"),
                             }),
                             ftxui::filler() }),
        ftxui::text(" "),
        ftxui::hbox({ ftxui::filler(),
          ftxui::vbox({ ftxui::hbox({ ftxui::text("Folder has been created in: "),
                          ftxui::text(fmt::format("songs/{}", song.name)) | ftxui::bold }),
            ftxui::text("Add a music file there and then press button. Song will play and you can press on the canvas"),
            ftxui::text("Each left click will be recorded and mapped so you can try your game later!"),
            ftxui::text(" "),
            inputs->Render() | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 60) }),
          ftxui::filler() }) });
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

    return ftxui::Renderer([&] {
      return ftxui::vbox({ ftxui::hbox({ ftxui::filler(),
                             ftxui::vbox({
                               ftxui::text(R"(             _     _                                               )"),
                               ftxui::text(R"(    /\      | |   | |                                              )"),
                               ftxui::text(R"(   /  \   __| | __| |  _ __   _____      __  ___  ___  _ __   __ _ )"),
                               ftxui::text(R"(  / /\ \ / _` |/ _` | | '_ \ / _ \ \ /\ / / / __|/ _ \| '_ \ / _` |)"),
                               ftxui::text(R"( / ____ \ (_| | (_| | | | | |  __/\ V  V /  \__ \ (_) | | | | (_| |)"),
                               ftxui::text(R"(/_/    \_\__,_|\__,_| |_| |_|\___| \_/\_/   |___/\___/|_| |_|\__, |)"),
                               ftxui::text(R"(                                                              __/ |)"),
                               ftxui::text(R"(                                                             |___/ )"),
                             }),
                             ftxui::filler() }),
        ftxui::text(" "),
        ftxui::hbox({ ftxui::filler(),
          ftxui::vbox({ ftxui::text("Music has been mapped and saved! You can go and play it now!"),
            ftxui::text(" "),
            inputs->Render() | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 60) }),
          ftxui::filler() }) });
    });
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
    case GameState::Scoreboard:
      return display_end_score();
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
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
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
