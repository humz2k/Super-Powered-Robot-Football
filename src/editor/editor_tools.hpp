#ifndef _SPRF_EDITOR_MODELS_HPP_
#define _SPRF_EDITOR_MODELS_HPP_

#include "engine/engine.hpp"

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

    public:
        Selectable(bool highlight = true) : m_id(next_id), m_highlight(highlight){
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
            Selectable::closest = (float)INFINITY;
            Selectable::closest_id = -1;
            //if (!IsMouseButtonPressed(0))return;
            //m_selected = false;
            //Selectable::currently_selected = NULL;
        }

        void update(){
            if (!IsMouseButtonPressed(0))return;
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
            if (!IsMouseButtonPressed(0))return;
            //TraceLog(LOG_INFO,"closest_id = %d",Selectable::closest_id);
            if (Selectable::closest_id == m_id){
                select();
            } else {
                unselect();
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

} // namespace SPRF

#endif // _SPRF_EDITOR_MODELS_HPP_