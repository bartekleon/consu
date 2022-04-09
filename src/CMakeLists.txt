find_package(fmt CONFIG)
find_package(spdlog CONFIG)
find_package(miniaudio CONFIG)

# Generic test that uses conan libs
add_executable(intro main.cpp "constants.hpp" "types.hpp" "utils.hpp" "audio.hpp")
target_link_libraries(
  intro
  PRIVATE project_options
          project_warnings
          fmt::fmt
          spdlog::spdlog
          miniaudio::miniaudio)

target_link_system_libraries(
  intro
  PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_include_directories(intro PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
