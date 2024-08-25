#include "crosshair.hpp"
#include "custom_mesh.hpp"
#include "editor/editor_tools.hpp"
#include "engine/engine.hpp"
#include "engine/sound.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_raylib.h"
#include "imgui/rlImGui.h"
#include "mouselook.hpp"
#include "networking/client.hpp"
#include "networking/map.hpp"
#include "networking/server.hpp"
#include "testing.hpp"
#include "animation.hpp"
#include <ik/ik.h>
#include <cassert>
#include <string>

namespace SPRF {

class ModelAnimationCommands : public DevConsoleCommand{
    private:
        ModelAnimator* m_animator;
    public:
        ModelAnimationCommands(DevConsole& console, ModelAnimator* animator) : DevConsoleCommand(console), m_animator(animator){

        }

        void handle(std::vector<std::string>& args){
            if (args.size() == 0)return;
            std::string name = args[0];
            for (int i = 1; i < args.size(); i++){
                name += " " + args[i];
            }
            m_animator->play_animation(name);
        }
};

class ModelAnimationEventCommand : public DevConsoleCommand{
    private:
        ModelAnimator* m_animator;
    public:
        ModelAnimationEventCommand(DevConsole& console, ModelAnimator* animator) : DevConsoleCommand(console), m_animator(animator){

        }

        void handle(std::vector<std::string>& args){
            if (args.size() == 0)return;
            std::string name = args[0];
            for (int i = 1; i < args.size(); i++){
                name += " " + args[i];
            }
            m_animator->event(name);
        }
};

class TestModelInput : public Component{
    private:
        ModelAnimator* m_animator;
        AnimationStateManager& m_manager;
    public:
        TestModelInput(ModelAnimator* animator) : m_animator(animator), m_manager(m_animator->state_manager()){

        }

        void update(){
            bool forward = IsKeyDown(KEY_T);
            bool backward = IsKeyDown(KEY_G);
            bool left = IsKeyDown(KEY_F);
            bool right = IsKeyDown(KEY_H);
            if (forward && !(backward || left || right)){
                m_manager.event("forward");
            }
            else if (backward && !(forward || left || right)){
                m_manager.event("backward");
            }
            else if (left && !(forward || backward || right)){
                m_manager.event("left");
            }
            else if (right && !(forward || backward || left)){
                m_manager.event("right");
            } else {
                m_manager.event("stop");
            }
        }
};

class MyScene : public TestScene {
  public:
    MyScene(Game* game) : TestScene(game, true) {
        //find_entity("IMGui Manager")->disable();
        auto player_model = this->renderer()->create_render_model("assets/xbot_rigged3.glb");

        auto player_entity = this->create_entity();
        //player_entity->add_component<AnimatedModel>("assets/xbot1.glb");
        auto player_model_entity = player_entity->create_child();
        auto player_model_component = player_model_entity->add_component<Model>(player_model);
        auto animator = player_model_entity->add_component<ModelAnimator>(player_model_entity,"assets/xbot_rigged3.glb",player_model_component);

        auto hands_model = this->renderer()->create_render_model("assets/xbot_hands.glb");
        auto hands_entity = find_entity("camera")->create_child("hands");
        auto hands_model_entity = hands_entity->create_child("hands_model");
        auto hands_model_component = hands_model_entity->add_component<Model>(hands_model);
        hands_model_entity->add_component<Selectable>(true,true);
        hands_entity->get_component<Transform>()->scale = vec3(0.01,0.01,0.01);
        hands_entity->get_component<Transform>()->rotation = vec3(M_PI_2,-0.7,0);
        hands_entity->get_component<Transform>()->position = vec3(-0.1,-1.7,0.2);
        hands_entity->add_component<Selectable>(true,true);
        auto model_animator = hands_model_entity->add_component<ModelAnimator>(hands_model_entity,"assets/xbot_hands.glb",hands_model_component);
        model_animator->play_animation("idle");
        //model_animator->stop_animation();

        auto gun_model = this->renderer()->create_render_model("assets/ak47.glb");
        auto gun_entity = player_model_entity->find_entity("mixamorig:RightHand")->create_child();
        gun_entity->add_component<Model>(gun_model);
        gun_entity->add_component<Selectable>(true,true);
        gun_entity->get_component<Transform>()->position = vec3(4,6,-18);
        gun_entity->get_component<Transform>()->rotation = vec3(-M_PI_2,0,M_PI_2);
        gun_entity->get_component<Transform>()->scale = vec3(20,20,20);

        auto gun_entity2 = hands_model_entity->find_entity("mixamorig:RightHand")->create_child();
        gun_entity2->add_component<Model>(gun_model);
        gun_entity2->add_component<Selectable>(true,true);
        gun_entity2->get_component<Transform>()->position = vec3(6,0,-18);
        gun_entity2->get_component<Transform>()->rotation = vec3(-M_PI_2,0.1,0.8);
        gun_entity2->get_component<Transform>()->scale = vec3(20,20,20);

        auto& anim_states = animator->state_manager();

        bool force_changes = true;
        anim_states.get_animation_state("strafe_left")->add_event("stop",anim_states.get_animation_state("idle"),force_changes);
        anim_states.get_animation_state("strafe_left")->add_event("forward",anim_states.get_animation_state("run_forward"),force_changes);
        anim_states.get_animation_state("strafe_left")->add_event("backward",anim_states.get_animation_state("run_backward"),force_changes);
        anim_states.get_animation_state("strafe_left")->add_event("right",anim_states.get_animation_state("strafe_right"),force_changes);

        anim_states.get_animation_state("strafe_right")->add_event("stop",anim_states.get_animation_state("idle"),force_changes);
        anim_states.get_animation_state("strafe_right")->add_event("forward",anim_states.get_animation_state("run_forward"),force_changes);
        anim_states.get_animation_state("strafe_right")->add_event("backward",anim_states.get_animation_state("run_backward"),force_changes);
        anim_states.get_animation_state("strafe_right")->add_event("left",anim_states.get_animation_state("strafe_left"),force_changes);

        anim_states.get_animation_state("run_forward")->add_event("stop",anim_states.get_animation_state("idle"),force_changes);
        anim_states.get_animation_state("run_forward")->add_event("right",anim_states.get_animation_state("strafe_right"),force_changes);
        anim_states.get_animation_state("run_forward")->add_event("backward",anim_states.get_animation_state("run_backward"),force_changes);
        anim_states.get_animation_state("run_forward")->add_event("left",anim_states.get_animation_state("strafe_left"),force_changes);

        anim_states.get_animation_state("run_backward")->add_event("stop",anim_states.get_animation_state("idle"),force_changes);
        anim_states.get_animation_state("run_backward")->add_event("right",anim_states.get_animation_state("strafe_right"),force_changes);
        anim_states.get_animation_state("run_backward")->add_event("forward",anim_states.get_animation_state("run_forward"),force_changes);
        anim_states.get_animation_state("run_backward")->add_event("left",anim_states.get_animation_state("strafe_left"),force_changes);

        anim_states.get_animation_state("idle")->add_event("forward",anim_states.get_animation_state("run_forward"),true);
        anim_states.get_animation_state("idle")->add_event("left",anim_states.get_animation_state("strafe_left"),true);
        anim_states.get_animation_state("idle")->add_event("right",anim_states.get_animation_state("strafe_right"),true);
        anim_states.get_animation_state("idle")->add_event("backward",anim_states.get_animation_state("run_backward"),true);

        animator->play_animation("idle");

        player_entity->add_component<TestModelInput>(animator);

        player_entity->get_component<Transform>()->scale = vec3(0.01,0.01,0.01);
        player_entity->get_component<Transform>()->rotation = vec3(M_PI_2,M_PI,0);
        player_model_entity->add_component<Selectable>(true,true);

        //animator->play_animation("sprint");
        dev_console()->add_command<ModelAnimationCommands>("play_animation",animator);
        dev_console()->add_command<ModelAnimationEventCommand>("animation_event",animator);

        dev_console()->exec("assets/editor/cfg/init.cfg");
    }
};

} // namespace SPRF

int main() {
    if(enet_initialize()){
        assert(1 == 0);
    }

    ik.init();

    int window_width = 1400;
    int window_height = 900;
    int render_width = 1400 * 2;
    int render_height = 900 * 2;
    int fps_max = 200;
    int fullscreen = 0;
    float volume = 1.0;

    SPRF::game =
        new SPRF::Game(window_width, window_height, "ik_test", render_width,
                       render_height, fps_max, fullscreen, volume);

    rlImGuiSetup(true);

    SPRF::game->load_scene<SPRF::MyScene>();

    while (SPRF::game->running()) {
        SPRF::game->draw();
    }

    delete SPRF::game;

    ik.deinit();
    rlImGuiShutdown();
    enet_deinitialize();
    return 0;
}