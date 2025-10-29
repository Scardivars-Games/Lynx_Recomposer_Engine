#include <ilen-game/triangle.h>

using namespace ilen::game;
using namespace ilen::core;
using namespace ilen::ecs::components;

void Triangle::Init() {
    ILEN_LOG_P("TRIANGLE", "Inicializado triángulo de prueba");
    
    //Inicializar los sistemas (En el motor normal, al leer los datos de la escena, ahí los iniciaría en vez de esta forma)
    //coordinator->registerComponent<TransformComponent>();
    //coordinator->registerComponent<RenderComponent>();

    TransformComponent transform{};
    transform.position = {0.0f, 0.0f, 0.0f};
    transform.scale = {1.0f, 1.0f, 1.0f};
    coordinator->addComponent(entity, std::move(transform));

    RenderComponent render{};
    render.meshID = 1;
    render.mainTexID = 0;
    render.materialID = 6;
    coordinator->addComponent(entity, std::move(render)); 
}

void Triangle::Update(){
//    ILEN_LOG_P("TRIANGLE", "Triangle update. Delta time: %.4f", ilen::time::Time::getDeltaTime());
}