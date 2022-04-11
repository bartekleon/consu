#include <fmt/printf.h>

#include "Game.hpp"

int main()
{
  try {
    Game game;
  } catch (const std::exception &e) {
    fmt::print("Unhandled exception in main: {}", e.what());
  }

  return 0;
}
