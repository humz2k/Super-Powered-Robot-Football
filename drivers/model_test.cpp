#include "raylib-cpp.hpp"
// #include <ik/ik.h>
#include <cassert>
#include <iostream>

int main() {
    /*raylib::Window window(900, 900, "test");

    SetTargetFPS(200);

    raylib::Camera3D camera(raylib::Vector3(0, 2, 10), raylib::Vector3(0, 0, 0),
                            raylib::Vector3(0, 1, 0), 45, CAMERA_PERSPECTIVE);

    raylib::Model model("assets/xbot1.glb");
    int animCount;
    ModelAnimation* anims = LoadModelAnimations("assets/xbot1.glb", &animCount);
    ModelAnimation current_anim = anims[2];

    ik_solver_t* solver = ik.solver.create(IK_FABRIK);
    //ik_node_t* root = solver->node->create(10000);
    std::vector<ik_node_t*> nodes;
    int nbones = current_anim.boneCount;
    nbones = 3;

    nodes.push_back(solver->node->create(0));
    for (int i = 1; i < nbones; i++){
        BoneInfo bone = current_anim.bones[i];
        nodes.push_back(solver->node->create_child(nodes[bone.parent],i));
        //nodes.push_back(solver->node->create_child(nodes[0],i));
        printf("%d parent = %d %s\n",i,bone.parent,bone.name);
    }
    for (int i = 0; i < nbones; i++){
        BoneInfo bone = current_anim.bones[i];
        printf("%d parent = %d %s\n",i,bone.parent,bone.name);
    }
    for (int i = 1; i < nbones; i++){
        //Transform transform = current_anim.framePoses[0][i];
        BoneInfo bone = current_anim.bones[i];
        int parent = bone.parent;
        //if (bone.parent != -1){
            //break;
        //    transform.translation =
    Vector3Subtract(transform.translation,current_anim.framePoses[0][bone.parent].translation);
        //    transform.rotation =
    raylib::Quaternion::FromMatrix(MatrixMultiply(raylib::Quaternion(transform.rotation).ToMatrix(),
    (raylib::Quaternion(current_anim.framePoses[0][bone.parent].rotation).Invert().ToMatrix())));
        //}

        //transform.translation = raylib::Vector3(transform.translation);
        nodes[i]->position =
    ik.vec3.vec3(1,0,0);//ik.vec3.vec3(transform.translation.x,transform.translation.y,transform.translation.z);
        //nodes[i]->rotation =
    ik.quat.quat(transform.rotation.x,transform.rotation.y,transform.rotation.z,transform.rotation.w);
        assert(parent != i);
        //if (parent == -1)
        printf("pos %d -> %d: %g %g
    %g\n",i,parent,nodes[i]->position.x,nodes[i]->position.y,nodes[i]->position.z);
        //else
        //    printf("pos %d: %g %g %g, %d, %g %g
    %g\n",i,nodes[i]->position.x,nodes[i]->position.y,nodes[i]->position.z,parent,current_anim.framePoses[0][parent].translation.x,current_anim.framePoses[0][parent].translation.y,current_anim.framePoses[0][parent].translation.z);
    }

    int eff = 2;

    ik_node_t* hand_node = nodes[eff];
    //raylib::Vector3 hand_pos(current_anim.framePoses[0][eff].translation);
    raylib::Vector3 hand_pos(1,1,1);
    //hand_pos -= raylib::Vector3(current_anim.framePoses[0][0].translation);

    ik_effector_t* effector = solver->effector->create();
    solver->effector->attach(effector,hand_node);
    //raylib::Quaternion q = current_anim.framePoses[0][eff].rotation;
    //q = raylib::Quaternion::FromMatrix(MatrixMultiply(q.ToMatrix(),
    (raylib::Quaternion(current_anim.framePoses[0][0].rotation).Invert().ToMatrix())));
    //effector->target_rotation = ik.quat.quat(q.x,q.y,q.z,q.w);

    solver->flags |= IK_ENABLE_TARGET_ROTATIONS;
    ik.solver.set_tree(solver, nodes[0]);

    raylib::Model cube_model(raylib::Mesh::Cube(0.1,0.1,0.1));

    while (!window.ShouldClose()) {

        if (IsKeyDown(KEY_F)){
            hand_pos.y -= GetFrameTime() * 10;
        }
        if (IsKeyDown(KEY_G)){
            hand_pos.y += GetFrameTime() * 10;
        }
        if (IsKeyDown(KEY_H)){
            hand_pos.z -= GetFrameTime() * 10;
        }
        if (IsKeyDown(KEY_J)){
            hand_pos.z += GetFrameTime() * 10;
        }

        effector->target_position =
    ik.vec3.vec3(hand_pos.x,hand_pos.y,hand_pos.z);


        ik.solver.rebuild(solver);
        ik.solver.solve(solver);

        /*for (int i = 0; i < nbones; i++){
            //ik_node_t* node = nodes[i];
            BoneInfo bone = current_anim.bones[i];
            int cur_parent = i;
            auto cur_pos = raylib::Vector3(0);
            while (cur_parent != -1){
                bone = current_anim.bones[cur_parent];
                auto p =
    raylib::Vector3(nodes[cur_parent]->position.x,nodes[cur_parent]->position.y,nodes[cur_parent]->position.z);
                cur_pos += p;
                cur_parent = bone.parent;
            }
            current_anim.framePoses[0][i].translation = cur_pos;
            //DrawCube(cur_pos * 0.01,0.05,0.05,0.05,GREEN);

        }

        UpdateModelAnimation(model, current_anim, 0);

        //current_frame += GetFrameTime() * 30;
        //if (((int)current_frame) >= current_anim.frameCount - 1) {
        //    current_frame = 0;
        //}
        BeginDrawing();
        ClearBackground(WHITE);
        camera.BeginMode();
        camera.Update(CAMERA_THIRD_PERSON);

        //DrawCube(,0.1,0.1,0.1,RED);
        auto q =
    raylib::Quaternion(effector->target_rotation.x,effector->target_rotation.y,effector->target_rotation.z,effector->target_rotation.w);
        auto [v,a] = q.ToAxisAngle();
        //printf("%g %g %g, %g\n",v.x,v.y,v.z,a);
        DrawModelEx(cube_model,raylib::Vector3(effector->target_position.x,effector->target_position.y,effector->target_position.z),v,a,raylib::Vector3(1.0,1.0,1.0),raylib::Color::Red());
        //std::cout <<
    raylib::Vector3(effector->target_position.x,effector->target_position.y,effector->target_position.z).Transform(mat_transform).ToString()
    << std::endl; std::vector<raylib::Vector3> world_positions; for (int i = 0;
    i < nbones; i++){
            //ik_node_t* node = nodes[i];
            BoneInfo bone = current_anim.bones[i];
            int cur_parent = i;
            auto cur_pos = raylib::Vector3(0);
            auto rot = raylib::Matrix::Identity();
            while (cur_parent != -1){
                bone = current_anim.bones[cur_parent];
                auto p =
    raylib::Vector3(nodes[cur_parent]->position.x,nodes[cur_parent]->position.y,nodes[cur_parent]->position.z);
                cur_pos += p;
                rot =
    MatrixMultiply(raylib::Quaternion(nodes[cur_parent]->rotation.x,nodes[cur_parent]->rotation.y,nodes[cur_parent]->rotation.z,nodes[cur_parent]->rotation.w).ToMatrix(),rot);
                cur_parent = bone.parent;
            }

            auto q = raylib::Quaternion::FromMatrix(rot);
            auto [v,a] = q.ToAxisAngle();
            auto tint = raylib::Color::Green();
            if (i == eff){
                tint = raylib::Color::Yellow();
            }
            world_positions.push_back(cur_pos);
            //printf("%g %g %g, %g\n",v.x,v.y,v.z,a);
            DrawModelEx(cube_model,cur_pos,v,a,raylib::Vector3(1.0,1.0,1.0),tint);
            DrawModel(cube_model,cur_pos,1.0,tint);
        }

        for (int i = 0; i < nbones; i++){
            BoneInfo bone = current_anim.bones[i];
            if (bone.parent == -1)continue;
            raylib::Vector3 pos = world_positions[i];
            raylib::Vector3 parent_pos = world_positions[bone.parent];
            DrawCylinderEx(pos,parent_pos,0.01,0.01,10,GREEN);
            int cur_parent = i;
            auto cur_pos = raylib::Vector3(0);
            auto rot = raylib::Matrix::Identity();
            while (cur_parent != -1){
                bone = current_anim.bones[cur_parent];
                auto p =
    raylib::Vector3(nodes[cur_parent]->position.x,nodes[cur_parent]->position.y,nodes[cur_parent]->position.z);
                cur_pos += p;
                rot =
    MatrixMultiply(raylib::Quaternion(nodes[cur_parent]->rotation.x,nodes[cur_parent]->rotation.y,nodes[cur_parent]->rotation.z,nodes[cur_parent]->rotation.w).ToMatrix(),rot);
                cur_parent = bone.parent;
            }

            auto q = raylib::Quaternion::FromMatrix(rot);
            auto [v,a] = q.ToAxisAngle();
            auto tint = raylib::Color::Green();
            if (i == eff){
                tint = raylib::Color::Yellow();
            }
            DrawModelEx(cube_model,cur_pos,v,a,raylib::Vector3(1.0,1.0,1.0),tint);
        }

        //model.Draw(raylib::Vector3(0, 0, 0), raylib::Vector3(1, 0, 0), 0,
        //           raylib::Vector3(1, 1, 1), raylib::Color(255,255,255,50));

        DrawGrid(10, 1);
        camera.EndMode();
        EndDrawing();
    }
    UnloadModelAnimations(anims, animCount);*/
    return 0;
}