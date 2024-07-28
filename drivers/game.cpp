#include "engine/engine.hpp"
#include "networking/client.hpp"
#include <string>

namespace SPRF {

class MenuScene;

class DisconnectCommand : public DevConsoleCommand {
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        dev_console().entity()->scene()->close();
    }
};

class Scene1 : public DefaultScene {
  public:
    Scene1(Game* game, std::string host, enet_uint32 port)
        : DefaultScene(game) {
        this->create_entity()->add_component<Client>(host, port);
        dev_console()->add_command<DisconnectCommand>("disconnect");
    }
    void on_close() { game()->load_scene<MenuScene>(); }
};

class ConnectCommand : public DevConsoleCommand {
  private:
    std::string* m_host;
    enet_uint32* m_port;

  public:
    ConnectCommand(DevConsole& dev_console, std::string* host,
                   enet_uint32* port)
        : DevConsoleCommand(dev_console), m_host(host), m_port(port) {}

    void handle(std::vector<std::string>& args) {
        *m_host = args[0].substr(0, args[0].find(":"));
        *m_port =
            std::stoi(args[0].substr(args[0].find(":") + 1, args[0].length()));
        dev_console().entity()->scene()->close();
    }
};

class MenuScene : public DefaultScene {
  private:
    std::string m_host;
    enet_uint32 m_port;

  public:
    MenuScene(Game* game) : DefaultScene(game) {
        dev_console()->add_command<ConnectCommand>("connect", &m_host, &m_port);
    }

    void on_close() {
        TraceLog(LOG_INFO, "connecting to %s:%u", m_host.c_str(), m_port);
        game()->load_scene<Scene1>(m_host, m_port);
    }
};

class Rotation : public Component {
  public:
    void update() {
        if (!IsKeyDown(KEY_Z)) {
            this->entity()->get_component<Transform>()->rotation.y -=
                GetMouseWheelMoveV().x * 0.2;
            this->entity()->get_component<Transform>()->rotation.x +=
                GetMouseWheelMoveV().y * 0.2;
        }
    }
};

class RotationKeysY : public Component {
  public:
    void update() {
        // if (!IsKeyDown(KEY_Z)){
        // this->entity()->get_component<Transform>()->rotation.x +=
        // GetFrameTime() * 0.5 * (IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
        this->entity()->get_component<Transform>()->rotation.y +=
            GetFrameTime() * 1 * (IsKeyDown(KEY_A) - IsKeyDown(KEY_D));
        //}
    }
};

class RotationKeysX : public Component {
  public:
    void update() {
        // if (!IsKeyDown(KEY_Z)){
        this->entity()->get_component<Transform>()->rotation.x +=
            GetFrameTime() * 1 * (IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
        // this->entity()->get_component<Transform>()->rotation.y +=
        // GetFrameTime() * 0.5 * (IsKeyDown(KEY_A) - IsKeyDown(KEY_D));
        //}
    }
};

class Zoom : public Component {
  public:
    void update() {
        if (IsKeyDown(KEY_Z)) {
            this->entity()->get_component<Transform>()->position.z +=
                GetMouseWheelMoveV().y;
            if (this->entity()->get_component<Transform>()->position.z > -1) {
                this->entity()->get_component<Transform>()->position.z = -1;
            }
        }
    }
};

class TestScene : public DefaultScene {
  public:
    TestScene(Game* game) : DefaultScene(game) {
        auto origin = this->create_entity();
        origin->add_component<Rotation>();
        origin->get_component<Transform>()->position.y = 0.5;
        auto camera = origin->create_child();
        camera->add_component<Zoom>();
        camera->get_component<Transform>()->position.z = -10;
        camera->get_component<Transform>()->position.y = 0;
        camera->add_component<Camera>();
        camera->get_component<Camera>()->set_active();
        auto light = this->renderer()->add_light();
        light->enabled(1);
        light->L(raylib::Vector3(0, 0, -1));

        auto head_model = this->renderer()->create_render_model(
            raylib::Mesh::Sphere(0.2, 30, 30));
        head_model->tint(raylib::Color::Red());
        auto body_model = this->renderer()->create_render_model(
            raylib::Mesh::Cone(0.2, 0.6, 100));
        body_model->tint(raylib::Color::Red());
        auto eye_model = this->renderer()->create_render_model(
            raylib::Mesh::Cube(0.2, 0.1, 0.1));
        eye_model->tint(raylib::Color::Black());
        auto arm_model = this->renderer()->create_render_model(
            raylib::Mesh::Cylinder(0.05, 0.4, 100));
        arm_model->tint(raylib::Color::Gray());

        auto player = this->create_entity();
        player->add_component<RotationKeysY>();
        player->get_component<Transform>()->position.y = 0;
        auto head = player->create_child();
        head->add_component<RotationKeysX>();
        head->get_component<Transform>()->position.y = 0.8;
        head->add_component<Model>(head_model);
        auto eye = head->create_child();
        eye->get_component<Transform>()->position.z = 0.15;
        eye->add_component<Model>(eye_model);
        auto body = player->create_child();
        body->add_component<Model>(body_model);
        body->get_component<Transform>()->position.y = 0.6;
        body->get_component<Transform>()->rotation.x = -M_PI;
        auto gun_parent = body->create_child();
        gun_parent->add_component<RotationKeysX>();
        auto gun = gun_parent->create_child();
        gun->add_component<Model>(arm_model);
        gun->get_component<Transform>()->position.z = -0.023;
        gun->get_component<Transform>()->position.x = 0.11;
        gun->get_component<Transform>()->position.y = 0.05;
        gun->get_component<Transform>()->rotation.x = -M_PI / 2;
        gun->get_component<Transform>()->rotation.y = 0.05;
        this->renderer()->load_skybox(
            "/Users/humzaqureshi/GitHub/Super-Powered-Robot-Football/src/"
            "defaultskybox.png");
        this->renderer()->enable_skybox();
    }
};

} // namespace SPRF

int main() {

    SPRF::Game game(900, 900, "test", 900 * 2, 900 * 2, 500);

    game.load_scene<SPRF::MenuScene>();

    while (game.running()) {
        game.draw();
    }

    return 0;
}