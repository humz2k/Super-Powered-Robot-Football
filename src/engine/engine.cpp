#include "engine.hpp"

namespace SPRF {

bool game_should_quit = false;

Game* game;
// LoadingScreen* loading_screen = NULL;

void quit() { game_should_quit = true; }

} // namespace SPRF