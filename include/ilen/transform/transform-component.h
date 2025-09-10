#pragma once
#include <ilen/headers/ecs.h>
#include <ilen/headers/maths.h>

//Componente que contiene los transforms de cierto objeto.
//Estos datos, en su formato más básico son: posición (vector3), rotación (vector3) y escala (vector3)
namespace ilen::ecs::components {
    
    using namespace ilen::maths;
    
    class TransformComponent {
        public:
            vec3 position = vec3(0.0f); //La posición inicial de momento será el origen en el mundo
            vec3 rotation = vec3(0.0f); //El objeto deve iniciar sin ninguna rotación

            vec3 localPosition = position;
            vec3 localRotation = rotation;
            vec3 scale = vec3(1.0f); //La escala será 1:1 con la fuente

            Entity parent = NULL;
            std::vector<Entity> childrens;

            matrix::mat4 getWorldMatrix();
        private:
            matrix::mat4 localToWorldMatrix;

        friend class TransformSystem;
    };
}