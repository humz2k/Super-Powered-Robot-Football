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
#include <ik/ik.h>
#include <cassert>
#include <string>

namespace SPRF {

//class IKSolver{
    //private:

//};

class ModelAnimator : public Component{
    private:
        Model* m_model;
        RenderModel* m_render_model;
        ::Model m_raylib_model;
        float m_frame_rate;
        int m_anim_count;
        ModelAnimation* m_anims;
        ModelAnimation m_my_anim;
        bool m_animation_playing = false;
        int m_current_animation = 0;
        float m_current_frame = 0;
        int m_last_frame = -1;
        std::unordered_map<std::string,int> m_animation_names;
        std::vector<Entity*> m_entity_bones;
        std::vector<Transform*> m_entity_transforms;
    public:
        ModelAnimator(std::string path, Model* model, std::string starting_animation = "TPose", float framerate = 60) : m_model(model), m_frame_rate(framerate){
            m_anims = LoadModelAnimations(path.c_str(),&m_anim_count);
            int bone_count = m_anims[0].boneCount;
            for (int i = 0; i < m_anim_count; i++){
                m_animation_names[std::string(m_anims[i].name)] = i;
                assert(bone_count == m_anims[i].boneCount);
                if (std::string(m_anims[i].name) == starting_animation){
                    m_current_animation = i;
                }
            }
            // hacky thing to be able to write different stuff not just each frame
            m_my_anim.boneCount = bone_count;
            m_my_anim.bones = m_anims[0].bones;
            m_my_anim.frameCount = 1;
            m_my_anim.framePoses = (::Transform**)malloc(sizeof(::Transform*));
            m_my_anim.framePoses[0] = (::Transform*)malloc(sizeof(::Transform) * bone_count);
            strcpy(m_my_anim.name,"base_anim");
        }

        ~ModelAnimator(){
            // free the hacks
            free(m_my_anim.framePoses[0]);
            free(m_my_anim.framePoses);
            UnloadModelAnimations(m_anims,m_anim_count);
        }

        void init(){
            m_render_model = m_model->render_model();
            m_raylib_model = *m_render_model->model();

            ModelAnimation cur_anim = m_anims[m_current_animation];
            //auto tmp = this->entity()->scene()->renderer()->create_render_model(Mesh::Sphere(5,10,10));
            for (int i = 0; i < m_anims[0].boneCount; i++){
                int parent = cur_anim.bones[i].parent;
                Entity* bone_entity;
                if (parent == -1){
                    bone_entity = this->entity()->create_child(cur_anim.bones[i].name);
                    bone_entity->get_component<Transform>()->position = cur_anim.framePoses[0][i].translation;
                } else {
                    bone_entity = m_entity_bones[parent]->create_child(cur_anim.bones[i].name);
                    bone_entity->get_component<Transform>()->position = vec3(cur_anim.framePoses[0][i].translation) - vec3(cur_anim.framePoses[0][parent].translation);
                }

                //bone_entity->add_component<Model>(tmp);
                bone_entity->add_component<Selectable>(true,true);
                m_entity_bones.push_back(bone_entity);
                m_entity_transforms.push_back(bone_entity->get_component<Transform>());
            }

            for (int i = 0; i < cur_anim.boneCount; i++){
                m_my_anim.framePoses[0][i] = cur_anim.framePoses[0][i];
            }

            UpdateModelAnimation(m_raylib_model,m_my_anim,0);
        }

        void update(){
            if (!m_animation_playing)return;
            ModelAnimation cur_anim = m_anims[m_current_animation];
            //int next_frame = m_current_frame;
            //if (next_frame == m_last_frame)return;

            int this_frame_idx = (int)m_current_frame;
            int next_frame_idx = (this_frame_idx + 1) % (cur_anim.frameCount-1);

            //UpdateModelAnimation(m_raylib_model,cur_anim,m_current_frame);
            //m_last_frame = m_current_frame;

            auto this_frame = cur_anim.framePoses[this_frame_idx];
            auto next_frame = cur_anim.framePoses[next_frame_idx];
            float lerp = m_current_frame - this_frame_idx;

            for (int i = 0; i < cur_anim.boneCount; i++){
                m_my_anim.framePoses[0][i].scale = Vector3Lerp(this_frame[i].scale,next_frame[i].scale,lerp);
                m_my_anim.framePoses[0][i].translation = Vector3Lerp(this_frame[i].translation,next_frame[i].translation,lerp);
                m_my_anim.framePoses[0][i].rotation = QuaternionSlerp(this_frame[i].rotation,next_frame[i].rotation,lerp);
            }

            for (int i = 0; i < cur_anim.boneCount; i++){
                int parent = cur_anim.bones[i].parent;
                if (cur_anim.bones[i].parent == -1){
                    m_entity_transforms[i]->position = m_my_anim.framePoses[0][i].translation;
                    continue;
                }
                m_entity_transforms[i]->position = vec3(m_my_anim.framePoses[0][i].translation) - vec3(m_my_anim.framePoses[0][parent].translation);
            }

            //auto my_inverse_global_transform = this->entity()->global_transform().Invert();

            UpdateModelAnimation(m_raylib_model,m_my_anim,0);

            m_current_frame += game_info.frame_time * m_frame_rate;
            m_current_frame = fmod(m_current_frame,m_anims[m_current_animation].frameCount-1);
        }

        void play_animation(int idx){
            m_animation_playing = true;
            m_current_animation = idx;
            m_current_frame = 0;
            m_last_frame = -1;
        }

        void play_animation(std::string name){
            if (!KEY_EXISTS(m_animation_names,name)){
                TraceLog(LOG_ERROR,"animation %s doesn't exist",name.c_str());
                return;
            }
            play_animation(m_animation_names[name]);
        }

        void stop_animation(){
            m_animation_playing = false;
        }
};

class ModelAnimationCommands : public DevConsoleCommand{
    private:
        ModelAnimator* m_animator;
    public:
        ModelAnimationCommands(DevConsole& console, ModelAnimator* animator) : DevConsoleCommand(console), m_animator(animator){

        }

        void handle(std::vector<std::string>& args){
            if (args.size() == 0)return;
            m_animator->play_animation(args[0]);
        }
};

class MyScene : public TestScene {
  public:
    MyScene(Game* game) : TestScene(game, true) {
        find_entity("IMGui Manager")->disable();
        auto player_model = this->renderer()->create_render_model("assets/xbot1.glb");
        auto player_entity = this->create_entity();
        //player_entity->add_component<AnimatedModel>("assets/xbot1.glb");
        auto player_model_component = player_entity->add_component<Model>(player_model);
        auto animator = player_entity->add_component<ModelAnimator>("assets/xbot1.glb",player_model_component);
        player_entity->get_component<Transform>()->scale = vec3(0.01,0.01,0.01);
        player_entity->get_component<Transform>()->rotation = vec3(M_PI_2,M_PI,0);
        player_entity->add_component<Selectable>(true,true);

        //animator->play_animation("sprint");
        dev_console()->add_command<ModelAnimationCommands>("play_animation",animator);

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