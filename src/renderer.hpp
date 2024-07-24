#ifndef _SPRF_RENDERER_HPP_
#define _SPRF_RENDERER_HPP_

#include "raylib-cpp.hpp"
#include "shaders.hpp"
#include <vector>
#include <iostream>

namespace SPRF{

class RenderModel{
    private:
        std::shared_ptr<raylib::Model> m_model;
        raylib::Matrix m_model_transform;
        std::vector<raylib::Matrix> m_instances;

        void clear_instances(){
            m_instances.clear();
        }

    public:
        RenderModel(std::shared_ptr<raylib::Model> model) : m_model(model), m_model_transform(raylib::Matrix(m_model->GetTransform())){
        }

        void add_instance(raylib::Matrix instance){
            m_instances.push_back(instance);
        }

        void draw(raylib::Shader& shader){
            Shader old_shader = m_model->materials[0].shader;
            m_model->materials[0].shader = shader;
            for (auto& transform : m_instances){
                raylib::Matrix final_transform = m_model_transform * transform;
                for (int i = 0; i < m_model->meshCount; i++){
                    DrawMesh(m_model->meshes[i],
                        m_model->materials[m_model->meshMaterial[i]], final_transform);
                }
            }
            m_model->materials[0].shader = old_shader;
            clear_instances();
        }
};


class Renderer{
    private:
        std::vector<std::shared_ptr<RenderModel>> m_render_models;
        raylib::Shader m_shader;
    public:
        Renderer() : m_shader(raylib::Shader::LoadFromMemory(lights_vs,lights_fs)){}

        std::shared_ptr<RenderModel> create_render_model(std::shared_ptr<raylib::Model> model){
            m_render_models.push_back(std::make_shared<RenderModel>(model));
            return m_render_models[m_render_models.size()-1];
        }

        raylib::Shader& shader(){
            return m_shader;
        }

        void render(){
            for (auto& i : m_render_models){
                i->draw(m_shader);
            }
        }
};

} // namespace SPRF

#endif // _SPRF_RENDERER_HPP_