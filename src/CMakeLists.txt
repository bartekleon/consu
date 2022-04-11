find_package(fmt CONFIG)
find_package(spdlog CONFIG)

# Generic test that uses conan libs
add_executable(intro main.cpp "constants.hpp" "types.hpp" "utils.hpp" "audio.hpp" "song.hpp" "Game.hpp")
target_link_libraries(
  intro
  PRIVATE project_options
          project_warnings
          fmt::fmt
          spdlog::spdlog
          )

target_link_system_libraries(
  intro
  PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component
  smk::smk)

target_include_directories(intro PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
