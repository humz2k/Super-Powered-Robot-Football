#ifndef _SPRF_EDITOR_MODELS_HPP_
#define _SPRF_EDITOR_MODELS_HPP_

#include "engine/engine.hpp"
#include "imgui/imgui.h"
#include "imgui/rlImGui.h"

namespace SPRF{

class Selectable : public Component{
    public:
        static Entity* currently_selected;
    private:
        static int next_id;
        static int closest_id;
        static float closest;
        bool m_selected = false;
        BBoxCorners m_bbox;
        int m_id;
        bool m_highlight;
        bool m_clickable;

    public:
        Selectable(bool highlight = true, bool clickable = false) : m_id(next_id), m_highlight(highlight), m_clickable(clickable){
            //TraceLog(LOG_INFO,"assigned id = %d",m_id);
            next_id++;
        }

        BoundingBox bounding_box(){
            return m_bbox.transform(this->entity()->global_transform()).axis_align();
        }

        void init(){
            if (this->entity()->has_component<Model>()){
                auto model = this->entity()->get_component<Model>()->render_model()->model();
                auto bbox = GetModelBoundingBox(*model);
                m_bbox = BBoxCorners(bbox);
            } else {
                memset(&m_bbox,0,sizeof(BBoxCorners));
            }
        }

        void before_update(){
            if (!m_clickable)return;
            Selectable::closest = (float)INFINITY;
            Selectable::closest_id = -1;
            //if (!IsMouseButtonPressed(0))return;
            //m_selected = false;
            //Selectable::currently_selected = NULL;
        }

        void update(){
            if (!m_clickable)return;
            if (!(IsMouseButtonPressed(0) && IsKeyDown(KEY_LEFT_SHIFT)))return;
            auto bbox = bounding_box();
            auto cam = this->entity()->scene()->get_active_camera();
            /*auto display_size = GetDisplaySize();
            auto render_size = game->render_size();
            auto display_aspect = display_size.x/display_size.y;
            auto render_aspect = render_size.x/render_size.y;
            auto render_corrected = raylib::Vector2(display_size.y * render_aspect,display_size.y);*/
            auto ray = cam->GetScreenToWorldRay((raylib::Vector2((GetMousePosition()))));
            auto collision = GetRayCollisionBox(ray,bbox);
            if (collision.hit){
                if (collision.distance < Selectable::closest){
                    Selectable::closest = collision.distance;
                    Selectable::closest_id = m_id;
                }
            }
        }

        void select(){
            if (Selectable::currently_selected){
                Selectable::currently_selected->get_component<Selectable>()->unselect();
            }
            m_selected = true;
            Selectable::currently_selected = this->entity();
            //TraceLog(LOG_INFO,"selected id %d",m_id);
        }

        void after_update(){
            if (!m_clickable)return;
            if (!(IsMouseButtonPressed(0) && IsKeyDown(KEY_LEFT_SHIFT)))return;
            //TraceLog(LOG_INFO,"closest_id = %d",Selectable::closest_id);
            if (Selectable::closest_id == m_id){
                select();
            } else {
                //unselect();
            }
        }

        void draw_debug(){
            if (m_selected && m_highlight)
                DrawBoundingBox(bounding_box(),GREEN);
        }

        bool selected(){return m_selected;}

        void unselect(){
            if (m_selected){
                Selectable::currently_selected = NULL;
                //TraceLog(LOG_INFO,"unselected id %d",m_id);
            }
            m_selected = false;
        }
};

class IMGuiManager : public Component{
    private:
        void parse_entity_tree(Entity* entity){
            std::string entity_name = "(" + std::to_string(entity->id()) + ") " + entity->name();

            if (ImGui::TreeNode(entity_name.c_str())){

                if (ImGui::IsItemToggledOpen()){
                    if (entity->has_component<Selectable>()){
                        entity->get_component<Selectable>()->select();
                    }
                } else if (ImGui::IsItemClicked()){
                    if (entity->has_component<Selectable>()){
                        if (entity->get_component<Selectable>()->selected()){
                            entity->get_component<Selectable>()->unselect();
                        }
                    }
                }
                for (int i = 0; i < entity->n_children(); i++){
                    parse_entity_tree(entity->get_child(i));
                }
                ImGui::TreePop();
            }
        }
    public:
        void before_draw2D(){
            rlImGuiBegin();
        }

        void after_draw2D(){
            rlImGuiEnd();
        }

        void draw2D(){
            if (game_info.dev_console_active)return;

            ImGui::Begin("Properties");
            if (Selectable::currently_selected){
                Entity* entity = Selectable::currently_selected;
                ImGui::Text("(%d) %s",entity->id(),entity->name().c_str());
                //float pos[] = {transform->position.x,transform->position.y,transform->position.z};
                ImGui::NewLine();
                entity->get_component<Transform>()->draw_editor();
                for (auto& i : entity->components()){
                    ImGui::NewLine();
                    i.second->draw_editor();
                }

            }
            ImGui::End();
            ImGui::Begin("hierarchy");
            for (auto& i : this->entity()->scene()->entities()){
                parse_entity_tree(i);
            }

            ImGui::End();
        }
};

} // namespace SPRF

#endif // _SPRF_EDITOR_MODELS_HPP_