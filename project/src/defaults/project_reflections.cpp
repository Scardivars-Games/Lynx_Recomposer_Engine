#include <game/components.h>
#include <lynx/resources/scene_manager.h>
#include <generated_serializers.inl>

namespace game {
    void RegisterProjectReflection(lynx::resources::scene::SceneManager& sceneManager){
        lynx::resources::scene::RegisterGeneratedDeserializers(sceneManager);
    }
}