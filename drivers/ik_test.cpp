#if 0
#include "crosshair.hpp"
#include "custom_mesh.hpp"
#include "engine/engine.hpp"
#include "engine/sound.hpp"
#include "mouselook.hpp"
#include "networking/client.hpp"
#include "networking/map.hpp"
#include "networking/server.hpp"
#include "testing.hpp"
#include <cassert>
#include <ik/ik.h>
#include <ik/transform.h>
#include <string>

namespace SPRF {

class IKSolver{
    private:
        ik_solver_t* m_raw;
        Entity* m_entity;
        RenderModel* m_node_model;
        RenderModel* m_effector_model;
        uint32_t id_count = 0;
    public:
        IKSolver(){}
        IKSolver(Entity* entity, ik_algorithm_e algo = IK_FABRIK) : m_raw(ik.solver.create(algo)), m_entity(entity) {
            m_node_model = m_entity->scene()->renderer()->create_render_model(raylib::Mesh::Cube(0.1,0.1,0.1));
            m_node_model->tint(raylib::Color::Green());
            m_effector_model = m_entity->scene()->renderer()->create_render_model(raylib::Mesh::Cube(0.1,0.1,0.1));
            m_effector_model->tint(raylib::Color::Red());
        }
        ik_solver_t* raw(){return m_raw;}
        Entity* entity(){return m_entity;}
        uint32_t next_id(){
            uint32_t out = id_count;
            id_count++;
            return out;
        }
        RenderModel* node_model(){return m_node_model;}
        RenderModel* effector_model(){return m_effector_model;}

};

class IKNode{
    private:
        IKSolver& m_solver;
        uint32_t m_id;
        ik_node_t* m_raw;
        std::vector<IKNode*> m_children;
        Entity* m_entity;
    public:
        IKNode* create_child(raylib::Vector3 pos = raylib::Vector3(0,0,0), raylib::Vector3 rot = raylib::Vector3(0,0,0)){
            IKNode* out = new IKNode(m_solver,this,pos,rot);
            m_children.push_back(out);
            return out;
        }
        IKNode(IKSolver& solver, raylib::Vector3 pos = raylib::Vector3(0,0,0), raylib::Vector3 rot = raylib::Vector3(0,0,0)) : m_solver(solver), m_id(m_solver.next_id()), m_raw(m_solver.raw()->node->create(m_id)){
            position(pos);
            rotation(rot);
            m_entity = solver.entity()->create_child();
            m_entity->add_component<Model>(solver.node_model());
            m_entity->get_component<Transform>()->position = position();
            m_entity->get_component<Transform>()->rotation = rotation();
            //m_raw->rotation_weight = 1.0f;
        }
        IKNode(IKSolver& solver, IKNode* parent, raylib::Vector3 pos = raylib::Vector3(0,0,0), raylib::Vector3 rot = raylib::Vector3(0,0,0)) : m_solver(solver), m_id(m_solver.next_id()), m_raw(m_solver.raw()->node->create_child(parent->raw(), m_id)){
            position(pos);
            rotation(rot);
            m_entity = parent->entity()->create_child();
            m_entity->add_component<Model>(solver.node_model());
            m_entity->get_component<Transform>()->position = position();
            m_entity->get_component<Transform>()->rotation = rotation();
            //m_raw->rotation_weight = 1.0f;
        }

        void update(){
            m_entity->get_component<Transform>()->position = position();
            m_entity->get_component<Transform>()->rotation = rotation();
            for (auto& i : m_children){
                i->update();
            }
        }

        ~IKNode(){
            for (auto& i : m_children){
                delete i;
            }
        }
        ik_node_t* raw(){return m_raw;}

        std::vector<IKNode*>& children(){
            return m_children;
        }

        raylib::Vector3 position(){
            return raylib::Vector3(m_raw->position.x,m_raw->position.y,m_raw->position.z);
        }

        raylib::Vector3 position(raylib::Vector3 pos){
            m_raw->position = ik.vec3.vec3(pos.x,pos.y,pos.z);
            return position();
        }

        raylib::Vector3 rotation(){
            raylib::Quaternion q(m_raw->rotation.x,m_raw->rotation.y,m_raw->rotation.z,m_raw->rotation.w);
            return q.ToEuler();
        }

        raylib::Vector3 rotation(raylib::Vector3 rot){
            auto q = raylib::Quaternion::FromEuler(rot);
            m_raw->rotation = ik.quat.quat(q.x,q.y,q.z,q.w);
            return rotation();
        }

        Entity* entity(){return m_entity;}
};

class IKEffector : public Component{
    private:
        IKSolver& m_solver;
        IKNode* m_node;
        ik_effector_t* m_raw;
        int m_chain_length;
        bool m_movable;
        bool m_diffed = true;
    public:
        IKEffector(IKSolver& solver, IKNode* node, int chain_length = 1, bool moveable = false) : m_solver(solver), m_node(node), m_raw(m_solver.raw()->effector->create()), m_chain_length(chain_length), m_movable(moveable){
            solver.raw()->effector->attach(m_raw,m_node->raw());
            m_raw->chain_length = m_chain_length;
        }

        void init(){
            position(this->entity()->get_component<Transform>()->position);
            rotation(this->entity()->get_component<Transform>()->rotation);
        }

        raylib::Vector3 position(){
            return raylib::Vector3(m_raw->target_position.x,m_raw->target_position.y,m_raw->target_position.z);
        }

        raylib::Vector3 position(raylib::Vector3 pos){
            m_raw->target_position = ik.vec3.vec3(pos.x,pos.y,pos.z);
            return position();
        }

        raylib::Vector3 rotation(){
            raylib::Quaternion q(m_raw->target_rotation.x,m_raw->target_rotation.y,m_raw->target_rotation.z,m_raw->target_rotation.w);
            return q.ToEuler();
        }

        raylib::Vector3 rotation(raylib::Vector3 rot){
            auto q = raylib::Quaternion::FromEuler(rot);
            m_raw->target_rotation = ik.quat.quat(q.x,q.y,q.z,q.w);
            return rotation();
        }

        void update(){
            if (m_movable){
                if (IsKeyDown(KEY_UP)){
                    if (IsKeyDown(KEY_P)){
                        this->entity()->get_component<Transform>()->rotation.z += game_info.frame_time;
                    }
                    else {
                        this->entity()->get_component<Transform>()->position.z += game_info.frame_time;
                    }
                    m_diffed = true;
                }
                if (IsKeyDown(KEY_DOWN)){
                    if (IsKeyDown(KEY_P)){
                        this->entity()->get_component<Transform>()->rotation.z -= game_info.frame_time;
                    }
                    else {
                        this->entity()->get_component<Transform>()->position.z -= game_info.frame_time;
                    }
                    m_diffed = true;
                }
                if (IsKeyDown(KEY_LEFT)){
                    if (IsKeyDown(KEY_P)){
                        this->entity()->get_component<Transform>()->rotation.x -= game_info.frame_time;
                    }
                    else {
                        this->entity()->get_component<Transform>()->position.x -= game_info.frame_time;
                    }
                    m_diffed = true;
                }
                if (IsKeyDown(KEY_RIGHT)){
                    if (IsKeyDown(KEY_P)){
                        this->entity()->get_component<Transform>()->rotation.x += game_info.frame_time;
                    }
                    else {
                        this->entity()->get_component<Transform>()->position.x += game_info.frame_time;
                    }
                    m_diffed = true;
                }
                if (IsKeyDown(KEY_M)){
                    this->entity()->get_component<Transform>()->position.y -= game_info.frame_time;
                    m_diffed = true;
                }
                if (IsKeyDown(KEY_N)){
                    this->entity()->get_component<Transform>()->position.y += game_info.frame_time;
                    m_diffed = true;
                }
            }

            position(this->entity()->get_component<Transform>()->position);
            rotation(this->entity()->get_component<Transform>()->rotation);
        }

        bool diff(){
            bool out = m_diffed;
            m_diffed = false;
            return out;
        }
};

class IKComponent : public Component{
    private:
        ik_algorithm_e m_algo;
        IKSolver m_solver;
        IKNode* m_root = NULL;
        std::vector<IKNode*> m_nodes;
        std::vector<Entity*> m_effectors;
        raylib::Model m_model;
        ModelAnimation* m_anims = NULL;
        int m_anim_count;
        IKNode* m_chest;
    public:
        IKComponent(ik_algorithm_e algo = IK_FABRIK) : m_algo(algo), m_model("assets/xbot1.glb"){
            m_anims = LoadModelAnimations("assets/xbot1.glb", &m_anim_count);
        }

        void init(){
            ModelAnimation current_anim = m_anims[2];

            m_solver = IKSolver(this->entity(),m_algo);
            m_solver.raw()->flags |= IK_ENABLE_TARGET_ROTATIONS | IK_ENABLE_JOINT_ROTATIONS;
            //m_solver.raw()->flags &= ~IK_ENABLE_TARGET_ROTATIONS;
            //m_solver.raw()->flags = 0;

            m_root = new IKNode(m_solver);
            m_chest = m_root->create_child(raylib::Vector3(0,2,0),raylib::Vector3(game_settings.float_values["chest_rot_x"],game_settings.float_values["chest_rot_y"],game_settings.float_values["chest_rot_z"]));
            /*auto pelvis = m_root->create_child(raylib::Vector3(0,-2,0));
            auto neck = chest->create_child(raylib::Vector3(0,0.5,0));
            auto head = neck->create_child(raylib::Vector3(0,0.5,0));
            auto left_shoulder = chest->create_child(raylib::Vector3(-0.2,0,0));*/
            auto right_shoulder = m_chest->create_child(raylib::Vector3(0.2,0,0),raylib::Vector3(0,0,0));
            auto right_arm = right_shoulder->create_child(raylib::Vector3(1,0,0));
            auto right_hand = right_arm->create_child(raylib::Vector3(1,0,0));
            /*auto left_arm = left_shoulder->create_child(raylib::Vector3(-1,0,0));
            auto left_hand = left_arm->create_child(raylib::Vector3(-1,0,0));*/

            /*for (int i = 0; i < current_anim.boneCount; i++){
                auto bone = current_anim.bones[i];
                TraceLog(LOG_INFO,"bone: %d : %s",i,bone.name);
                raylib::Vector3 pos = raylib::Vector3(current_anim.framePoses[0][i].translation) * 0.01;
                raylib::Quaternion rot = current_anim.framePoses[0][i].rotation;
                //raylib::Vector3 parent_pos;
                //raylib::Vector3 out_rot = raylib::Vector3(0,M_PI,0);//rot.ToEuler();
                //if (bone.parent == -1){
                //    parent_pos = pos;
                    //out_rot = rot.ToEuler();//raylib::Vector3(0,0,0);
                //} else {
                //    parent_pos = current_anim.framePoses[0][bone.parent].translation;
                    //raylib::Quaternion parent_rot = current_anim.framePoses[0][bone.parent].rotation;
                    //out_rot = rot.ToEuler() - parent_rot.ToEuler();// - parent_rot.ToEuler();//raylib::Quaternion::FromMatrix(raylib::Matrix(rot.ToMatrix()) * raylib::Matrix(parent_rot.Invert().ToMatrix()) ).ToEuler();
                //}
                //pos = pos - parent_pos;
                //pos *= 0.01;
                //TraceLog(LOG_INFO,"%d: rot %g %g %g\n",i,out_rot.x,out_rot.y,out_rot.z);
                if (bone.parent == -1){
                    //m_nodes.push_back(m_root->create_child(pos,rot.ToEuler()));
                    m_nodes.push_back(m_root->create_child(pos));
                } else {
                    //m_nodes.push_back(m_nodes[bone.parent]->create_child(pos,rot.ToEuler()));
                    m_nodes.push_back(m_nodes[bone.parent]->create_child(pos));
                }
            }*/

            //m_nodes.push_back(new IKNode(m_solver));
            //auto child1 = m_nodes[0]->create_child(raylib::Vector3(0,0,1));
            //auto child2 = child1->create_child(raylib::Vector3(0,0,1));


            auto effector = this->entity()->create_child();//new IKEffector(m_solver,child2,raylib::Vector3(1,1,1));
            m_effectors.push_back(effector);
            effector->add_component<IKEffector>(m_solver,right_hand,2,true);
            effector->add_component<Model>(m_solver.effector_model());
            effector->get_component<Transform>()->position = raylib::Vector3(0,0,0).Transform(right_hand->entity()->global_transform());

            /*int effs[] = {6};
            for (int i = 0; i < sizeof(effs)/sizeof(int); i++){
                auto effector = this->entity()->create_child();//new IKEffector(m_solver,child2,raylib::Vector3(1,1,1));
                m_effectors.push_back(effector);
                effector->add_component<IKEffector>(m_solver,m_nodes[effs[i]],2,true);
                effector->add_component<Model>(m_solver.effector_model());
                effector->get_component<Transform>()->position = (raylib::Vector3(current_anim.framePoses[0][effs[i]].translation)) * 0.01;
            }*/

            /*int effs2[] = {34};
            for (int i = 0; i < sizeof(effs)/sizeof(int); i++){
                auto effector = this->entity()->create_child();//new IKEffector(m_solver,child2,raylib::Vector3(1,1,1));
                m_effectors.push_back(effector);
                effector->add_component<IKEffector>(m_solver,m_nodes[effs2[i]],1,false);
                effector->add_component<Model>(m_solver.effector_model());
                effector->get_component<Transform>()->position = (raylib::Vector3(current_anim.framePoses[0][effs2[i]].translation) - raylib::Vector3(current_anim.framePoses[0][0].translation)) * 0.01;
            }*/

            //int effs3[] = {10};
            //for (int i = 0; i < sizeof(effs3)/sizeof(int); i++){
            //    auto effector = this->entity()->create_child();//new IKEffector(m_solver,child2,raylib::Vector3(1,1,1));
            //    m_effectors.push_back(effector);
            //    effector->add_component<IKEffector>(m_solver,m_nodes[effs3[i]],2,true);
            //    effector->add_component<Model>(m_solver.effector_model());
            //    effector->get_component<Transform>()->position = (raylib::Vector3(current_anim.framePoses[0][effs3[i]].translation)) * 0.01;
            //}

            ik.solver.set_tree(m_solver.raw(), m_root->raw());
            //ik_transform_tree(m_root->raw(),TR_G2L);
            //ik.transform.global_to_local(m_solver.raw()->tree);

            ik.solver.rebuild(m_solver.raw());
            ik.solver.solve(m_solver.raw());
        }

        void update(){
            m_chest->rotation(raylib::Vector3(game_settings.float_values["chest_rot_x"],game_settings.float_values["chest_rot_y"],game_settings.float_values["chest_rot_z"]));

            bool solve = true;
            /*for (auto& i : m_effectors){
                if (i->get_component<IKEffector>()->diff()){
                    solve = true;
                    break;
                }
            }*/
            if (solve){
                //ik.solver.rebuild(m_solver.raw());
                ik.solver.solve(m_solver.raw());
            }

            //if (m_nodes.size() > 0){
            m_root->update();
            //}

            /*ModelAnimation current_anim = m_anims[2];
            for (int i = 0; i < current_anim.boneCount; i++){
                if (i == 6){
                    auto tmp = raylib::Vector3(0,0,0).Transform(m_nodes[i]->entity()->global_transform()) / 0.01;
                    TraceLog(LOG_INFO,"%g %g %g\n",tmp.x,tmp.y,tmp.z);
                }
                current_anim.framePoses[0][i].translation = raylib::Vector3(0,0,0).Transform(m_nodes[i]->entity()->global_transform()) / 0.01;
            }
            UpdateModelAnimation(m_model,current_anim,0);*/
        }

        void draw_debug(){
            /*ModelAnimation current_anim = m_anims[2];
            for (int i = 0; i < current_anim.boneCount; i++){
                //if (i == 6){
                //    auto tmp = raylib::Vector3(0,0,0).Transform(m_nodes[i]->entity()->global_transform()) / 0.01;
                    //TraceLog(LOG_INFO,"%g %g %g\n",tmp.x,tmp.y,tmp.z);
                //}
                current_anim.framePoses[1][i].translation = raylib::Vector3(0,0,0).Transform(m_nodes[i]->entity()->global_transform()) / 0.01;
                current_anim.framePoses[1][i].rotation = raylib::Quaternion::FromMatrix(raylib::Matrix(raylib::Quaternion(current_anim.framePoses[0][i].rotation).ToMatrix()) * m_nodes[i]->entity()->global_rotation());//raylib::Quaternion::FromMatrix(m_nodes[i]->entity()->global_rotation());//
            }
            UpdateModelAnimation(m_model,current_anim,1);
            m_model.DrawWires(raylib::Vector3(0,0,0),0.01,raylib::Color::White());*/
        }

        void destroy(){
            //if (m_nodes.size() > 0)
            delete m_root;
            UnloadModelAnimations(m_anims,m_anim_count);
        }
};

class MyScene : public TestScene{
    public:
        MyScene(Game* game) : TestScene(game){
            this->create_entity()->add_component<IKComponent>(IK_MSS);
        }
};

}

int main(){
    assert(enet_initialize() == 0);

    int window_width = 800;
    int window_height = 800;
    int render_width = 800 * 2;
    int render_height = 800 * 2;
    int fps_max = 200;
    int fullscreen = 0;
    float volume = 1.0;
    ik.init();
    ik.log.init();

    SPRF::game =
        new SPRF::Game(window_width, window_height, "ik_test", render_width,
                       render_height, fps_max, fullscreen, volume);

    SPRF::game->load_scene<SPRF::MyScene>();

    while (SPRF::game->running()) {
        SPRF::game->draw();
    }

    delete SPRF::game;
    ik.deinit();
    enet_deinitialize();
    return 0;
}
#endif

int main() { return 0; }