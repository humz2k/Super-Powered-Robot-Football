#include "crosshair.hpp"
#include "custom_mesh.hpp"
#include "engine/engine.hpp"
#include "engine/sound.hpp"
#include "mouselook.hpp"
#include "networking/client.hpp"
#include "networking/map.hpp"
#include "networking/server.hpp"
#include "animation.hpp"
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
    //Transform* m_head_transform;
    NetworkEntity* m_network_entity;
    bool m_enabled = true;

  public:
    void init() {
        m_transform = this->entity()->get_component<Transform>();
        //m_head_transform =
        //    this->entity()->get_child(0)->get_component<Transform>();
        m_network_entity = this->entity()->get_component<NetworkEntity>();
    }

    void update() {
        m_transform->position = m_network_entity->position;
        m_transform->rotation.y = m_network_entity->rotation.y;
        //m_head_transform->rotation.x = m_network_entity->rotation.x;
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
    auto player_model = player->scene()->renderer()->create_render_model("assets/xbot_rigged3.glb");

    player->add_component<PlayerComponent>();
    auto player_model_entity = player->create_child();
    auto player_model_model = player_model_entity->add_component<Model>(player_model);
    player_model_entity->get_component<Transform>()->scale = vec3(0.01,0.01,0.01) * PLAYER_HEIGHT;
    player_model_entity->get_component<Transform>()->rotation = vec3(M_PI_2,0,0);
    player_model_entity->get_component<Transform>()->position = vec3(0,-0.5,0);
    auto animator = player_model_entity->add_component<ModelAnimator>(player_model_entity,"assets/xbot_rigged3.glb",player_model_model);
    animator->play_animation("idle");

    auto gun_model = player->scene()->renderer()->create_render_model("assets/ak47.glb");
    auto gun_entity = player_model_entity->find_entity("mixamorig:RightHand")->create_child();
    gun_entity->add_component<Model>(gun_model);
    //gun_entity->add_component<Selectable>(true,true);
    gun_entity->get_component<Transform>()->position = vec3(4,6,-18);
    gun_entity->get_component<Transform>()->rotation = vec3(-M_PI_2,0,M_PI_2);
    gun_entity->get_component<Transform>()->scale = vec3(20,20,20);
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

class GameScene : public DefaultScene{
    private:
        Client* m_client = NULL;
    public:
        GameScene(Game* game, std::string host = "127.0.0.1", enet_uint16 port = 31201) : DefaultScene(game){
            Map("assets/maps/simple_map.json").load(this);

            auto player = this->create_entity();
            player->add_component<Crosshair>();
            m_client = player->add_component<Client>(host, port,
                                                    init_player, dev_console());
            auto camera = player->create_child();;
            camera->add_component<Camera>()->set_active();
            auto hands_model = this->renderer()->create_render_model("assets/xbot_hands.glb");
            auto hands_entity = camera->create_child("hands");
            auto hands_model_entity = hands_entity->create_child("hands_model");
            auto hands_model_component = hands_model_entity->add_component<Model>(hands_model);
            //hands_model_entity->add_component<Selectable>(true,true);
            hands_entity->get_component<Transform>()->scale = vec3(0.01,0.01,0.01);
            hands_entity->get_component<Transform>()->rotation = vec3(M_PI_2,-0.7,0);
            hands_entity->get_component<Transform>()->position = vec3(-0.1,-1.7,0.2);
            hands_entity->add_component<Selectable>(true,true);
            auto model_animator = hands_model_entity->add_component<ModelAnimator>(hands_model_entity,"assets/xbot_hands.glb",hands_model_component);
            model_animator->play_animation("idle");

            auto gun_model = this->renderer()->create_render_model("assets/ak47.glb");

            auto gun_entity2 = hands_model_entity->find_entity("mixamorig:RightHand")->create_child();
            gun_entity2->add_component<Model>(gun_model);
            gun_entity2->add_component<Selectable>(true,true);
            gun_entity2->get_component<Transform>()->position = vec3(6,0,-18);
            gun_entity2->get_component<Transform>()->rotation = vec3(-M_PI_2,0.1,0.8);
            gun_entity2->get_component<Transform>()->scale = vec3(20,20,20);

            player->get_child(0)->add_component<MouseLook>();
            player->get_child(0)->add_component<SoundListener>();

            std::function<void()> callback = [this]() { disconnect(); };
            dev_console()->add_command<DisconnectCommand>("disconnect", callback);
        }

        void disconnect() { this->close(); }

        void on_close() { game()->load_scene<MenuScene>(); }

        virtual ~GameScene(){
            m_client->close();
        }
};

class LocalServer{
    private:
        SPRF::Server m_server;
    public:
        LocalServer(std::string cfg, std::string host = "127.0.0.1", enet_uint16 port = 31201) : m_server(cfg, host, port){ }

        virtual ~LocalServer(){
            m_server.quit();
            m_server.join();
        }
};

class LocalScene : public LocalServer, public GameScene {
  public:
    LocalScene(Game* game)
        : LocalServer("server_cfg.ini"), GameScene(game)
        {

        dev_console()->add_command<LocalSceneServerCommands>("server");

        dev_console()->exec("assets/server/local/cfg/init.cfg");
    }
};

class Scene1 : public GameScene{
    public:
        Scene1(Game* game, std::string host, enet_uint32 port) : GameScene(game,host,port){}
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