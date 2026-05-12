#include <ilen-public/API/camera-api.h>
#include <ilen/window/window.h>

using namespace ilen::ecs;
using namespace ilen::engine::api;
using namespace ilen::maths;
using namespace ilen::ecs::components;

void CameraAPI::Init(const ilen::EngineContext* context) {
    p_coordinator = context->p_coordinator.get();
    p_cameraSystem = p_coordinator->getSystem<ilen::ecs::systems::CameraSystem>().get();
}

//Necesita ejecutarse antes que cualquier script que lo consulte.
void CameraAPI::GetMainCameraPointer() {
    if(mainCamera == nullptr)
        mainCamera = p_cameraSystem->getMainCamera();
}

void CameraAPI::ResetMainCameraPointer() {
    mainCamera = nullptr;
}

//Toma la screenPos, y la normaliza a [-1, 1]
vec2 CameraAPI::ScreenToNDC(const vec2& screenPos){
    //Obtener tamaño de la ventana
    auto screenSize = vec2(ilen::window::IWindow::getWidth(), ilen::window::IWindow::getHeight());
    return vec2(
        (2.0f * screenPos.x / screenSize.x) - 1.0f, 
        (2.0f * screenPos.y / screenSize.y) - 1.0f
    );
}

vec3 CameraAPI::NDCToWorld(const CameraComponent* camera, const vector::Vector2& ndcPos){
    matrix::mat4 invVP = matrix::inverse(camera->viewProjectionMatrix);

    vec4 clipPos(ndcPos.x, ndcPos.y, 0.0f, 1.0f);
    vec4 worldPos4 = invVP * clipPos;

    //Division de perspectiva
    if(std::abs(worldPos4.w) > 0.0000001f) {
        worldPos4.x /= worldPos4.w;
        worldPos4.y /= worldPos4.w;
        worldPos4.z /= worldPos4.w;
    }

    return vec3(worldPos4.x, worldPos4.y, worldPos4.z);
}

//Funciones públicas 
vec2 CameraAPI::ConvertScreenToNDC(const vec2& screenPos){
    return ScreenToNDC(screenPos);
}

vec3 CameraAPI::ConvertNDCToWorld(const ilen::ecs::components::CameraComponent* camera, const vec2& ndcPos){
    return NDCToWorld(camera, ndcPos);
}

vec3 CameraAPI::ConvertScreenToWorld(const ilen::ecs::components::CameraComponent* camera, const vec2& screenPos){
    return NDCToWorld(camera, ScreenToNDC(screenPos));
}