#include "engine/engine.hpp"
#include "networking/client.hpp"
#include "custom_mesh.hpp"
#include <string>
#include <cassert>

namespace SPRF {

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
        this->entity()->get_component<Transform>()->rotation.y +=
            GetFrameTime() * 1 * (IsKeyDown(KEY_A) - IsKeyDown(KEY_D));
    }
};

class RotationKeysX : public Component {
  public:
    void update() {
        this->entity()->get_component<Transform>()->rotation.x +=
            GetFrameTime() * 1 * (IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
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
        m_display_w = GetRenderWidth();
        m_display_h = GetRenderHeight();
        if (IsKeyPressed(KEY_Q)){
            if (mouse_locked){
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
        if (!mouse_locked){
            return;
        }

        //float aspect = ((float)GetDisplayWidth())/((float)GetDisplayHeight());
        //float fovx = 2 * atan(tan(DEFAULT_FOVY * 0.5) * aspect);
        //float deg_per_pix = fovx/((float)GetDisplayWidth());
        auto mouse_delta = raylib::Vector2(GetMouseDelta())*(1/0.2765)*(360.0f/16363.6364)*DEG2RAD*sense;// * game_info.mouse_sense_ratio * sense;// * (deg_per_pix);
        this->entity()->get_component<Transform>()->rotation.x += mouse_delta.y;
        this->entity()->get_component<Transform>()->rotation.y -= mouse_delta.x;

        this->entity()->get_component<Transform>()->rotation.y =
            Wrap(this->entity()->get_component<Transform>()->rotation.y, 0,
                 2 * M_PI);
        this->entity()->get_component<Transform>()->rotation.x =
            Clamp(this->entity()->get_component<Transform>()->rotation.x,
                  -M_PI * 0.5f + 0.5, M_PI * 0.5f - 0.5);
    }

    void draw2D(){
        game_info.draw_debug_var("display",raylib::Vector3(GetWindowScaleDPI().x,GetWindowScaleDPI().y,0),500,500);
    }
};

class PlayerComponent : public Component{
    private:
        Transform* m_transform;
        Transform* m_head_transform;
        NetworkEntity* m_network_entity;
    public:
        void init(){
            m_transform = this->entity()->get_component<Transform>();
            m_head_transform = this->entity()->get_child(0)->get_component<Transform>();
            m_network_entity = this->entity()->get_component<NetworkEntity>();
        }

        void update(){
            m_transform->position = m_network_entity->position;
            m_transform->rotation.y = m_network_entity->rotation.y;
            m_head_transform->rotation.x = m_network_entity->rotation.x;
        }

        void draw2D(){
            game_info.draw_debug_var("net_rotation",m_network_entity->rotation,300,300);
        }
};

void init_player(Entity* player){
    TraceLog(LOG_INFO,"initializing player");
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

class Scene1 : public DefaultScene {
  public:
    Scene1(Game* game, std::string host, enet_uint32 port)
        : DefaultScene(game) {

        int map_x_size = 70;
        int map_z_size = 60;

        auto plane = this->renderer()->create_render_model(WrappedMesh(map_x_size,map_z_size,10,10));
        plane->add_texture("assets/prototype_texture/grey4.png");
        plane->clip(false);
        auto plane_entity = this->create_entity();
        plane_entity->add_component<Model>(plane);

        auto cube = this->renderer()->create_render_model(raylib::Mesh::Cube(1,1,1));
        cube->add_texture("assets/prototype_texture/orange-cube.png");

        auto sphere = this->renderer()->create_render_model(raylib::Mesh::Sphere(0.5,30,30));
        //cube->add_texture("assets/prototype_texture/orange-cube.png");

        auto sphere1 = this->create_entity();
            sphere1->get_component<Transform>()->position.y = 0.5;
            sphere1->get_component<Transform>()->position.z = 0.5;// + (float)10;
            sphere1->get_component<Transform>()->rotation.y = 0;
            sphere1->add_component<Model>(sphere);

        auto sphere2 = this->create_entity();
            sphere2->get_component<Transform>()->position.y = 0.5;
            sphere2->get_component<Transform>()->position.z = 0.5 + (float)2;
            sphere2->get_component<Transform>()->rotation.y = M_PI_2;
            sphere2->add_component<Model>(sphere);

        for (int i = -(map_z_size/2); i < (map_z_size/2); i++){
            for (int y = 0; y < 5; y++){
                auto cube_entity = this->create_entity();
                cube_entity->get_component<Transform>()->position.y = 0.5 + (float)y;
                cube_entity->get_component<Transform>()->position.z = 0.5 + (float)i;
                cube_entity->get_component<Transform>()->position.x = 0.5 + -((float)map_x_size)*0.5;
                cube_entity->add_component<Model>(cube);

                auto cube_entity2 = this->create_entity();
                cube_entity2->get_component<Transform>()->position.y = 0.5 + (float)y;
                cube_entity2->get_component<Transform>()->position.z = 0.5 + (float)i;
                cube_entity2->get_component<Transform>()->position.x = 0.5 + ((float)map_x_size)*0.5;
                cube_entity2->add_component<Model>(cube);
            }
        }

        for (int i = -(map_x_size/2)+1; i < (map_x_size/2); i++){
            for (int y = 0; y < 5; y++){
                auto cube_entity = this->create_entity();
                cube_entity->get_component<Transform>()->position.y = 0.5 + (float)y;
                cube_entity->get_component<Transform>()->position.x = 0.5 + (float)i;
                cube_entity->get_component<Transform>()->position.z = 0.5 + -((float)map_z_size/2);
                cube_entity->add_component<Model>(cube);

                auto cube_entity2 = this->create_entity();
                cube_entity2->get_component<Transform>()->position.y = 0.5 + (float)y;
                cube_entity2->get_component<Transform>()->position.x = 0.5 + (float)i;
                cube_entity2->get_component<Transform>()->position.z = 0.5 + ((float)map_z_size/2)-1.0;
                cube_entity2->add_component<Model>(cube);
            }
        }

        auto player = this->create_entity();
        player->add_component<Client>(host, port,init_player);
        auto camera = player->create_child()->add_component<Camera>();
        camera->set_active();

        player->get_child(0)->add_component<MouseLook>();

        std::function<void()> callback = [this]() { disconnect(); };
        dev_console()->add_command<DisconnectCommand>("disconnect", callback);

        auto light = this->renderer()->add_light();
        light->enabled(1);
        light->L(raylib::Vector3(1, 2, 0.02));
        light->target(raylib::Vector3(2.5,0,0));
        light->fov(70);
        this->renderer()->load_skybox("src/"
                                      "defaultskybox.png");
        this->renderer()->enable_skybox();
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

class MenuScene : public DefaultScene {
  public:
    MenuScene(Game* game) : DefaultScene(game) {
        std::function<void(std::string, enet_uint32)> callback =
            [this](std::string host, enet_uint32 port) { connect(host, port); };
        dev_console()->add_command<ConnectCommand>("connect", callback);
    }

    void connect(std::string host, enet_uint32 port) {
        TraceLog(LOG_INFO, "MenuScene connect called...");
        game()->load_scene<Scene1>(host, port);
    }
};

} // namespace SPRF

int main() {
    assert(enet_initialize() == 0);
    // SPRF::game = new SPRF::Game(1512, 982, "test", 1512 * 2, 982 * 2, 200);
    SPRF::game = new SPRF::Game(0, 0, "test", 1024 * 2, 768 * 2, 200);
    ToggleFullscreen();

    SPRF::game->load_scene<SPRF::Scene1>("192.168.1.73",9999);

    while (SPRF::game->running()) {
        SPRF::game->draw();
    }

    delete SPRF::game;
    enet_deinitialize();
    return 0;
}