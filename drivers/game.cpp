#include "custom_mesh.hpp"
#include "engine/engine.hpp"
#include "networking/client.hpp"
#include "networking/map.hpp"
#include "networking/server.hpp"
#include <cassert>
#include <string>

namespace SPRF {

class MenuScene;

class DisconnectCommand : public DevConsoleCommand {
  private:
    std::function<void()> m_callback;

  public:
    DisconnectCommand(DevConsole& dev_console, std::function<void()> callback)
        : DevConsoleCommand(dev_console), m_callback(callback) {}

    void handle(std::vector<std::string>& args) { m_callback(); }
};

class MouseLook : public Component {
  private:
    float sense = 2;
    bool mouse_locked = false;
    float m_display_w;
    float m_display_h;

  public:
    void init() {}

    void update() {
        if (IsKeyPressed(KEY_Q)) {
            if (mouse_locked) {
                EnableCursor();
                mouse_locked = false;
            } else {
                DisableCursor();
                mouse_locked = true;
            }
        }
        if (game_info.dev_console_active) {
            if (mouse_locked) {
                EnableCursor();
                mouse_locked = false;
            }
            return;
        }
        if (!mouse_locked) {
            return;
        }

        auto mouse_delta =
            GetRawMouseDelta() * game_info.mouse_sense_ratio * sense;
        this->entity()->get_component<Transform>()->rotation.x += mouse_delta.y;
        this->entity()->get_component<Transform>()->rotation.y -= mouse_delta.x;

        this->entity()->get_component<Transform>()->rotation.y =
            Wrap(this->entity()->get_component<Transform>()->rotation.y, 0,
                 2 * M_PI);
        this->entity()->get_component<Transform>()->rotation.x =
            Clamp(this->entity()->get_component<Transform>()->rotation.x,
                  -M_PI * 0.5f + 0.5, M_PI * 0.5f - 0.5);
    }
};

class PlayerComponent : public Component {
  private:
    Transform* m_transform;
    Transform* m_head_transform;
    NetworkEntity* m_network_entity;

  public:
    void init() {
        m_transform = this->entity()->get_component<Transform>();
        m_head_transform =
            this->entity()->get_child(0)->get_component<Transform>();
        m_network_entity = this->entity()->get_component<NetworkEntity>();
    }

    void update() {
        m_transform->position = m_network_entity->position;
        m_transform->rotation.y = m_network_entity->rotation.y;
        m_head_transform->rotation.x = m_network_entity->rotation.x;
    }

    void draw2D() {
        game_info.draw_debug_var("net_rotation", m_network_entity->rotation,
                                 300, 300);
    }
};

void init_player(Entity* player) {
    TraceLog(LOG_INFO, "initializing player");
    auto head_model = player->scene()->renderer()->create_render_model(
        raylib::Mesh::Sphere(0.2, 30, 30));
    head_model->tint(raylib::Color::Red());
    auto body_model = player->scene()->renderer()->create_render_model(
        raylib::Mesh::Cone(0.2, 0.6, 100));
    body_model->tint(raylib::Color::Red());
    auto eye_model = player->scene()->renderer()->create_render_model(
        raylib::Mesh::Cube(0.2, 0.1, 0.1));
    eye_model->tint(raylib::Color::Black());
    auto arm_model = player->scene()->renderer()->create_render_model(
        raylib::Mesh::Cylinder(0.05, 0.4, 100));
    arm_model->tint(raylib::Color::Gray());

    player->add_component<PlayerComponent>();
    player->get_component<Transform>()->position.y = 0;
    auto head = player->create_child();
    head->get_component<Transform>()->position.y = 0.3;
    head->add_component<Model>(head_model);
    auto eye = head->create_child();
    eye->get_component<Transform>()->position.z = 0.15;
    eye->add_component<Model>(eye_model);
    auto body = player->create_child();
    body->add_component<Model>(body_model);
    body->get_component<Transform>()->position.y = 0.1;
    body->get_component<Transform>()->rotation.x = -M_PI;
    auto gun_parent = body->create_child();
    auto gun = gun_parent->create_child();
    gun->add_component<Model>(arm_model);
    gun->get_component<Transform>()->position.z = -0.023;
    gun->get_component<Transform>()->position.x = 0.11;
    gun->get_component<Transform>()->position.y = 0.05;
    gun->get_component<Transform>()->rotation.x = -M_PI / 2;
    gun->get_component<Transform>()->rotation.y = 0.05;
}

class LocalScene : public DefaultScene {
  private:
    SPRF::Server m_server;
    Client* m_client;

  public:
    LocalScene(Game* game)
        : DefaultScene(game), m_server("/Users/humzaqureshi/GitHub/Super-Powered-Robot-Football/server_config.ini", "127.0.0.1", 9999) {

        int map_x_size = 70;
        int map_z_size = 60;

        simple_map()->load(this);

        auto player = this->create_entity();
        m_client = player->add_component<Client>("127.0.0.1", 9999, init_player,
                                                 dev_console());
        auto camera = player->create_child()->add_component<Camera>();
        camera->set_active();

        player->get_child(0)->add_component<MouseLook>();

        std::function<void()> callback = [this]() { disconnect(); };
        dev_console()->add_command<DisconnectCommand>("disconnect", callback);
    }

    ~LocalScene() {
        m_client->close();
        m_server.quit();
        m_server.join();
    }

    void disconnect() { this->close(); }

    void on_close() { game()->load_scene<MenuScene>(); }
};

class Scene1 : public DefaultScene {
  public:
    Scene1(Game* game, std::string host, enet_uint32 port)
        : DefaultScene(game) {

        int map_x_size = 70;
        int map_z_size = 60;

        simple_map()->load(this);

        auto player = this->create_entity();
        player->add_component<Client>(host, port, init_player, dev_console());
        auto camera = player->create_child()->add_component<Camera>();
        camera->set_active();

        player->get_child(0)->add_component<MouseLook>();

        std::function<void()> callback = [this]() { disconnect(); };
        dev_console()->add_command<DisconnectCommand>("disconnect", callback);
    }
    void disconnect() { this->close(); }

    void on_close() { game()->load_scene<MenuScene>(); }
};

class ConnectCommand : public DevConsoleCommand {
  private:
    std::function<void(std::string, enet_uint32)> m_callback;

  public:
    ConnectCommand(DevConsole& dev_console,
                   std::function<void(std::string, enet_uint32)> callback)
        : DevConsoleCommand(dev_console), m_callback(callback) {}

    void handle(std::vector<std::string>& args) {
        std::string m_host = args[0].substr(0, args[0].find(":"));
        enet_uint32 m_port =
            std::stoi(args[0].substr(args[0].find(":") + 1, args[0].length()));
        m_callback(m_host, m_port);
    }
};

class ConnectLocalCommand : public DevConsoleCommand {
  private:
    std::function<void()> m_callback;

  public:
    ConnectLocalCommand(DevConsole& dev_console, std::function<void()> callback)
        : DevConsoleCommand(dev_console), m_callback(callback) {}

    void handle(std::vector<std::string>& args) { m_callback(); }
};

class MenuScene : public DefaultScene {
  public:
    MenuScene(Game* game) : DefaultScene(game) {
        std::function<void(std::string, enet_uint32)> callback =
            [this](std::string host, enet_uint32 port) { connect(host, port); };
        std::function<void()> callback_local = [this]() { connect_local(); };
        dev_console()->add_command<ConnectCommand>("connect", callback);
        dev_console()->add_command<ConnectLocalCommand>("connect_local",
                                                        callback_local);
    }

    void connect(std::string host, enet_uint32 port) {
        TraceLog(LOG_INFO, "MenuScene connect called...");
        game()->load_scene<Scene1>(host, port);
    }

    void connect_local() {
        TraceLog(LOG_INFO, "MenuScene connect local called...");
        game()->load_scene<LocalScene>();
    }
};

} // namespace SPRF

int main() {
    assert(enet_initialize() == 0);
    SPRF::game = new SPRF::Game(900, 900, "test", 900 * 2, 900 * 2, 200);
    // SPRF::game = new SPRF::Game(1600, 900, "test", 1024*2, 768*2, 200);
    //   ToggleFullscreen();

    // SPRF::game->load_scene<SPRF::MenuScene>();
    SPRF::game->load_scene<SPRF::MenuScene>();

    while (SPRF::game->running()) {
        SPRF::game->draw();
    }

    delete SPRF::game;
    enet_deinitialize();
    return 0;
}