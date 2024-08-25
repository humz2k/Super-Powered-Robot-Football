#ifndef _SPRF_ANIMATION_HPP_
#define _SPRF_ANIMATION_HPP_

#include "engine/engine.hpp"

namespace SPRF{

class AnimationState{
    private:
        ModelAnimation m_anim;
        std::string m_animation_name;
        bool m_loop;
        bool m_playing = false;
        std::unordered_map<std::string,AnimationState*> m_actions;
        std::unordered_map<std::string,bool> m_force;
        float m_current_frame = 0;
        float m_frame_rate;
        vec3 m_rotation = vec3(0,0,0);
        bool m_currently_looping = false;
        AnimationState* m_next = NULL;

    public:
        AnimationState(ModelAnimation anim, bool loop = true, float frame_rate = 60.0f) : m_anim(anim), m_animation_name(std::string(m_anim.name)), m_loop(loop), m_frame_rate(frame_rate){

        }

        float frame_rate(){
            return m_frame_rate;
        }

        float frame_rate(float rate){
            m_frame_rate = rate;
            return frame_rate();
        }

        int bone_count(){
            return m_anim.boneCount;
        }

        BoneInfo* bones(){
            return m_anim.bones;
        }

        AnimationState* play(){
            m_current_frame = 0;
            m_playing = true;
            m_currently_looping = m_loop;
            if (KEY_EXISTS(m_actions,"next")){
                m_next = m_actions["next"];
            }
            return this;
        }

        AnimationState* stop(){
            m_current_frame = 0;
            m_playing = false;
            m_currently_looping = false;
            m_next = NULL;
            return this;
        }

        AnimationState* event(std::string event_name){
            if (!KEY_EXISTS(m_actions,event_name)){
                return this;
            }
            //m_playing = false;
            m_next = m_actions[event_name];
            m_currently_looping = false;
            if (m_playing && (!m_force[event_name])){
                return this;
            } /*else if (m_playing){
                m_current_frame = m_anim.frameCount - 3;
                return this;
            }*/
            return m_next->play();
        }

        void add_event(std::string event_name, AnimationState* state, bool force = false){
            m_actions[event_name] = state;
            m_force[event_name] = force;
        }

        bool loop(){
            return m_loop;
        }

        bool loop(bool l){
            m_loop = l;
            return loop();
        }

        ::Transform** frame_poses(){
            return m_anim.framePoses;
        }

        AnimationState* update_animation(ModelAnimation to_update){
            if (!m_playing)return this;
            int this_frame_idx = (int)m_current_frame;
            int next_frame_idx = (this_frame_idx + 1) % (m_anim.frameCount-1);

            auto this_frame = m_anim.framePoses[this_frame_idx];
            auto next_frame = m_anim.framePoses[next_frame_idx];

            if ((this_frame_idx == (m_anim.frameCount - 2)) && !m_currently_looping){
                if (m_next){
                    //TraceLog(LOG_INFO,"getting next frame?");
                    next_frame = m_next->frame_poses()[0];
                }
            }

            float lerp = m_current_frame - this_frame_idx;

            for (int i = 0; i < m_anim.boneCount; i++){
                to_update.framePoses[0][i].scale = Vector3Lerp(this_frame[i].scale,next_frame[i].scale,lerp);
                to_update.framePoses[0][i].translation = Vector3Lerp(this_frame[i].translation,next_frame[i].translation,lerp);
                to_update.framePoses[0][i].rotation = QuaternionSlerp(this_frame[i].rotation,next_frame[i].rotation,lerp);
            }

            m_current_frame += game_info.frame_time * m_frame_rate;
            AnimationState* next = this;
            if (!m_currently_looping){
                if (m_current_frame >= (float)(m_anim.frameCount - 1)){
                    m_playing = false;
                    if (m_next){
                        next = m_next->play();
                    }
                }
            }
            m_current_frame = fmod(m_current_frame,m_anim.frameCount - 1);
            return next;
        }

        std::string name(){
            return m_animation_name;
        }

        vec3 rotation(){
            return m_rotation;
        }

        vec3 rotation(vec3 rot){
            m_rotation = rot;
            return rotation();
        }
};

class AnimationStateManager{
    private:
        std::unordered_map<std::string,AnimationState*> m_states;
        AnimationState* m_playing = NULL;
        bool m_initialized = false;
        ModelAnimation m_cur_anim;
    public:
        AnimationStateManager(){
        }

        void add_animation_state(ModelAnimation anim, bool loop = true, float frame_rate = 60.0f){
            auto state = new AnimationState(anim,loop,frame_rate);
            assert(!KEY_EXISTS(m_states,state->name()));
            m_states[state->name()] = state;
            if (m_playing == NULL){
                m_playing = state->play();
            }
            if (!m_initialized){
                m_cur_anim.boneCount = state->bone_count();
                m_cur_anim.bones = state->bones();
                m_cur_anim.frameCount = 1;
                m_cur_anim.framePoses = (::Transform**)malloc(sizeof(::Transform*));
                m_cur_anim.framePoses[0] = (::Transform*)malloc(sizeof(::Transform) * state->bone_count());
                strcpy(m_cur_anim.name,"base_anim");
                for (int i = 0; i < m_cur_anim.boneCount; i++){
                    m_cur_anim.framePoses[0][i] = state->frame_poses()[0][i];
                }
                m_initialized = true;
            }
        }

        AnimationState* get_animation_state(std::string name){
            assert(KEY_EXISTS(m_states,name));
            return m_states[name];
        }

        ~AnimationStateManager(){
            for (auto& i : m_states){
                delete i.second;
            }
            if (m_initialized){
                free(m_cur_anim.framePoses[0]);
                free(m_cur_anim.framePoses);
            }
        }

        ModelAnimation update(){
            if (m_playing != NULL){
                m_playing = m_playing->update_animation(m_cur_anim);
            }
            return m_cur_anim;
        }

        void event(std::string event_name){
            if (m_playing != NULL){
                m_playing = m_playing->event(event_name);
            }
        }

        vec3 rotation(){
            if (m_playing){
                return m_playing->rotation();
            }
            return vec3(0,0,0);
        }

        void play_animation(std::string name){
            if (KEY_EXISTS(m_states,name)){
                if (m_playing){
                    m_playing->stop();
                }
                m_playing = m_states[name]->play();
            }
        }

        void stop_animation(){
            if (m_playing){
                m_playing->stop();
            }
            m_playing = NULL;
        }
};

class ModelAnimator : public Component{
    private:
        Model* m_model;
        RenderModel* m_render_model;
        ::Model m_raylib_model;
        int m_anim_count;
        ModelAnimation* m_anims;
        std::vector<Entity*> m_entity_bones;
        std::vector<Transform*> m_entity_transforms;
        AnimationStateManager anim_states;
    public:
        ModelAnimator(std::string path, Model* model, std::string starting_animation = "TPose", float framerate = 60) : m_model(model){
            m_anims = LoadModelAnimations(path.c_str(),&m_anim_count);
            int bone_count = m_anims[0].boneCount;
            for (int i = 0; i < m_anim_count; i++){
                assert(bone_count == m_anims[i].boneCount);
                anim_states.add_animation_state(m_anims[i],true,framerate);
            }
            anim_states.play_animation(starting_animation);
        }

        ModelAnimator(Entity* entity, std::string path, Model* model, std::string starting_animation = "TPose", float framerate = 60) : m_model(model){
            m_anims = LoadModelAnimations(path.c_str(),&m_anim_count);
            int bone_count = m_anims[0].boneCount;
            for (int i = 0; i < m_anim_count; i++){
                assert(bone_count == m_anims[i].boneCount);
                anim_states.add_animation_state(m_anims[i],true,framerate);
            }
            anim_states.play_animation(starting_animation);
            ModelAnimation updated_anim = anim_states.update();

            for (int i = 0; i < updated_anim.boneCount; i++){
                auto bone = updated_anim.bones[i];
                Entity* bone_entity;
                if (bone.parent == -1){
                    bone_entity = entity->create_child(bone.name);
                    bone_entity->get_component<Transform>()->position = vec3(updated_anim.framePoses[0][i].translation);
                } else {
                    bone_entity = m_entity_bones[bone.parent]->create_child(bone.name);
                    bone_entity->get_component<Transform>()->position = vec3(updated_anim.framePoses[0][i].translation) - vec3(updated_anim.framePoses[0][bone.parent].translation);
                }
                //bone_entity->add_component<Model>(tmp);
                //bone_entity->add_component<Selectable>(true,true);

                m_entity_transforms.push_back(bone_entity->get_component<Transform>());
                m_entity_bones.push_back(bone_entity);
                //auto bone_entity =
                //m_entity_bones.push_back(this->entity()->c)
            }
        }

        AnimationStateManager& state_manager(){
            return anim_states;
        }

        ~ModelAnimator(){
            UnloadModelAnimations(m_anims,m_anim_count);
        }

        void init(){
            m_render_model = m_model->render_model();
            m_raylib_model = *m_render_model->model();
            ModelAnimation updated_anim = anim_states.update();
            UpdateModelAnimation(m_raylib_model, updated_anim, 0);
            this->entity()->get_component<Transform>()->rotation = anim_states.rotation();
            //auto tmp = this->entity()->scene()->renderer()->create_render_model(Mesh::Sphere(5,10,10));
            if (m_entity_bones.size() == 0){
                for (int i = 0; i < updated_anim.boneCount; i++){
                    auto bone = updated_anim.bones[i];
                    Entity* bone_entity;
                    if (bone.parent == -1){
                        bone_entity = this->entity()->create_child(bone.name);
                        bone_entity->get_component<Transform>()->position = vec3(updated_anim.framePoses[0][i].translation);
                    } else {
                        bone_entity = m_entity_bones[bone.parent]->create_child(bone.name);
                        bone_entity->get_component<Transform>()->position = vec3(updated_anim.framePoses[0][i].translation) - vec3(updated_anim.framePoses[0][bone.parent].translation);
                    }
                    //bone_entity->add_component<Model>(tmp);
                    //bone_entity->add_component<Selectable>(true,true);

                    m_entity_transforms.push_back(bone_entity->get_component<Transform>());
                    m_entity_bones.push_back(bone_entity);
                    //auto bone_entity =
                    //m_entity_bones.push_back(this->entity()->c)
                }
            }
        }

        void update(){
            ModelAnimation updated_anim = anim_states.update();
            UpdateModelAnimation(m_raylib_model, updated_anim, 0);
            this->entity()->get_component<Transform>()->rotation = anim_states.rotation();
            for (int i = 0; i < updated_anim.boneCount; i++){
                auto bone = updated_anim.bones[i];
                Transform* bone_transform = m_entity_transforms[i];
                if (bone.parent == -1){
                    bone_transform->position = vec3(updated_anim.framePoses[0][i].translation);
                } else {
                    bone_transform->position = vec3(updated_anim.framePoses[0][i].translation) - vec3(updated_anim.framePoses[0][bone.parent].translation);
                }
            }
        }

        void play_animation(std::string name){
            anim_states.play_animation(name);
        }

        void event(std::string name){
            anim_states.event(name);
        }

        void stop_animation(){
            anim_states.stop_animation();
        }
};

} // namespace SPRF

#endif // _SPRF_ANIMATION_HPP_