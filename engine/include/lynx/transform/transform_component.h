#pragma once
#include <lynx/headers/ecs.h>
#include <lynx/headers/maths.h>

namespace lynx::ecs::systems {
    class TransformSystem;
}

//Componente que contiene los transforms de cierto objeto.
//Estos datos, en su formato más básico son: posición (vector3), rotación (vector3) y escala (vector3)

namespace lynx::ecs::components {
    using namespace lynx::maths;
    
    LYNX_COMPONENT
    struct TransformComponent {
        public:
            LYNX_PROPERTY vec3 position = vec3(0.0f); //La posición inicial de momento será el origen en el mundo
            LYNX_PROPERTY vec3 rotation = vec3(0.0f); //El objeto deve iniciar sin ninguna rotación

            LYNX_PROPERTY vec3 localPosition = position;
            LYNX_PROPERTY vec3 localRotation = rotation;
            LYNX_PROPERTY vec3 scale = vec3(1.0f); //La escala será 1:1 con la fuente

            LYNX_PROPERTY Entity parent = NULL_ENTITY;
            LYNX_PROPERTY std::vector<Entity> childrens;

            //matrix::mat4 getWorldMatrix() const { return localToWorldMatrix; }
        //private:
            LYNX_PROPERTY matrix::mat4 localToWorldMatrix;

        //friend class lynx::ecs::systems::TransformSystem;
    };

    //Generado en tiempo de ejecucion, no serializar.
    struct TransformOrder {
        vec3 translation = vec3(0.0f);
        vec3 rotation = vec3(0.0f);
        vec3 scaleMultiplier = vec3(1.0f);

        bool hasOrders = false;
        bool isNewPos = false; //Si es true, el valor de translation es el valor del transform final, no es una traslacion
        bool isNewRot = false; //Si es true, el valor de la rotacion es directamente escena
        bool isNewScale = false; //Si es true, el valor del multiplier es la nueva escala. Si es false, multiplica a la escala anterior.
    };
}