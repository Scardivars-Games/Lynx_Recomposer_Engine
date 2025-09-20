#include <ilen-game/triangle.h>

using namespace ilen::game;
using namespace ilen::core;
using namespace ilen::ecs::components;

void Triangle::Init() {
    Debug::Log("Inicializado triángulo de prueba");
    
    //Inicializar los sistemas (En el motor normal, al leer los datos de la escena, ahí los iniciaría en vez de esta forma)
    //coordinator->registerComponent<TransformComponent>();
    //coordinator->registerComponent<RenderComponent>();

    TransformComponent transform{};
    transform.position = {0.0f, 0.0f, 0.0f};
    transform.scale = {1.0f, 1.0f, 1.0f};
    coordinator->addComponent(entity, transform);

    RenderComponent render{};
    render.meshID = 1;
    render.mainTexID = 0;
    render.materialID = 6;
    coordinator->addComponent(entity, render); 
}

void Triangle::Update(){
    
    Debug::Log("Triangle update. Delta time: " + std::to_string(ilen::time::Time::getDeltaTime()));
}