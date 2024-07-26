#include "engine.hpp"

namespace SPRF {

bool game_should_quit = false;

void quit() { game_should_quit = true; }

GameInfo game_info;

} // namespace SPRF