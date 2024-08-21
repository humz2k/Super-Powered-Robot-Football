#include "crosshair.hpp"
#include "custom_mesh.hpp"
#include "engine/engine.hpp"
#include "engine/sound.hpp"
#include "mouselook.hpp"
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

class PlayerComponent : public Component {
  private:
    Transform* m_transform;
    Transform* m_head_transform;
    NetworkEntity* m_network_entity;
    bool m_enabled = true;

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
        if (m_network_entity->active && (!m_enabled)) {
            for (auto& i : this->entity()->children()) {
                TraceLog(LOG_INFO, "enabling");
                i->enable();
            }
            m_enabled = true;
        } else if ((!m_network_entity->active) && (m_enabled)) {
            for (auto& i : this->entity()->children()) {
                TraceLog(LOG_INFO, "disabling");
                i->disable();
            }
            m_enabled = false;
        }
    }

    void draw2D() {
        // game_info.draw_debug_var("net_rotation", m_network_entity->rotation,
        //                          300, 300);
    }
};

void init_player(Entity* player) {
    TraceLog(LOG_INFO, "initializing player");
    auto head_model = player->scene()->renderer()->create_render_model(
        Mesh::Sphere(0.2, 30, 30));
    head_model->tint(Color::Red());
    auto body_model = player->scene()->renderer()->create_render_model(
        Mesh::Cone(0.2, 0.6, 100));
    body_model->tint(Color::Red());
    auto eye_model = player->scene()->renderer()->create_render_model(
        Mesh::Cube(0.2, 0.1, 0.1));
    eye_model->tint(Color::Black());
    auto arm_model = player->scene()->renderer()->create_render_model(
        Mesh::Cylinder(0.05, 0.4, 100));
    arm_model->tint(Color::Gray());

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

class LocalSceneServerCommands : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        if (args.size() == 0)
            return;
        if (args[0] == "set_ball_position") {
            if (args.size() != 4)
                return;
            std::string lua_command = "sprf.set_ball_position(" + args[1] +
                                      "," + args[2] + "," + args[3] + ")";
            scripting.run_string(lua_command);
            return;
        }
        if (args[0] == "set_ball_velocity") {
            if (args.size() != 4)
                return;
            std::string lua_command = "sprf.set_ball_velocity(" + args[1] +
                                      "," + args[2] + "," + args[3] + ")";
            scripting.run_string(lua_command);
            return;
        }
        if (args[0] == "reset_game") {
            scripting.run_string("reset_game()");
            return;
        }
    }
};

class LocalScene : public DefaultScene {
  private:
    SPRF::Server m_server;
    Client* m_client;

  public:
    LocalScene(Game* game)
        : DefaultScene(game), m_server("server_cfg.ini", "127.0.0.1", 31201) {

        // simple_map()->load(this);
        Map("assets/maps/simple_map.json").load(this);

        auto player = this->create_entity();
        player->add_component<Crosshair>();
        m_client = player->add_component<Client>("127.0.0.1", 31201,
                                                 init_player, dev_console());
        auto camera = player->create_child()->add_component<Camera>();
        camera->set_active();

        player->get_child(0)->add_component<MouseLook>();
        player->get_child(0)->add_component<SoundListener>();

        std::function<void()> callback = [this]() { disconnect(); };
        dev_console()->add_command<DisconnectCommand>("disconnect", callback);

        dev_console()->add_command<LocalSceneServerCommands>("server");

        dev_console()->exec("assets/server/local/cfg/init.cfg");
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

        Map("assets/maps/simple_map.json").load(this);

        auto player = this->create_entity();
        player->add_component<Crosshair>();
        player->add_component<Client>(host, port, init_player, dev_console());
        auto camera = player->create_child()->add_component<Camera>();
        camera->set_active();

        player->get_child(0)->add_component<MouseLook>();
        player->get_child(0)->add_component<SoundListener>();

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
    if(enet_initialize()){
        assert(1 == 0);
    }

    int window_width = 0;
    int window_height = 0;
    int render_width = 1920;
    int render_height = 1080;
    int fps_max = 200;
    int fullscreen = 1;
    float volume = 1.0;
    ChangeDirectory(GetApplicationDirectory());
    mINI::INIFile file("client_cfg.ini");
    mINI::INIStructure ini;
    if (file.read(ini)) { // Ensure the INI file is successfully read
        if (ini.has("display")) {
#define DUMB_HACK(field, token)                                                \
    if (field.has(TOSTRING(token))) {                                          \
        token = std::stoi(field[TOSTRING(token)]);                             \
        field[TOSTRING(token)] = std::to_string(token);                        \
    }
            auto& display = ini["display"];
            DUMB_HACK(display, window_width);
            DUMB_HACK(display, window_height);
            DUMB_HACK(display, render_width);
            DUMB_HACK(display, render_height);
            DUMB_HACK(display, fps_max);
            DUMB_HACK(display, fullscreen);
#undef DUMB_HACK
        }
        if (ini.has("sound")) {
#define DUMB_HACK(field, token)                                                \
    if (field.has(TOSTRING(token))) {                                          \
        token = std::stof(field[TOSTRING(token)]);                             \
        field[TOSTRING(token)] = std::to_string(token);                        \
    }
            auto& sound = ini["sound"];
            DUMB_HACK(sound, volume);
#undef DUMB_HACK
        }
    }

    SPRF::game =
        new SPRF::Game(window_width, window_height, "SPRF", render_width,
                       render_height, fps_max, fullscreen, volume);
    // SPRF::game = new SPRF::Game("client_cfg.ini");
    //  SPRF::game = new SPRF::Game(1600, 900, "test", 1024*2, 768*2, 200);

    // SPRF::game->load_scene<SPRF::MenuScene>();
    SPRF::game->load_scene<SPRF::MenuScene>();

    while (SPRF::game->running()) {
        SPRF::game->draw();
    }

    delete SPRF::game;
    enet_deinitialize();
    return 0;
}