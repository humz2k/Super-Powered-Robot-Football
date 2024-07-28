#include "engine/engine.hpp"
#include "networking/client.hpp"
#include <string>

namespace SPRF {

class Scene1 : public DefaultScene {
  public:
    Scene1(Game* game) : DefaultScene(game) {
        this->create_entity()->add_component<Client>("127.0.0.1", 9999);
    }
};
} // namespace SPRF

int main() {

    SPRF::Game game(900, 900, "test", 900, 900, 500);

    game.load_scene<SPRF::Scene1>();

    while (game.running()) {
        game.draw();
    }

    return 0;
}