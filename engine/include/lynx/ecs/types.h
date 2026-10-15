//Entity-Component system, tipos básicos de cada entidad.
#pragma once
#include <cstdint>
#include <bitset>
#include <random>
#include <limits>
#include <lynx/headers/enumerations.h>
#include <lynx/ecs/components_registry.h>

using TagSystem = lynx::enumerations::LYNX_TAG_SYSTEM;
using LayerSystem = lynx::enumerations::LYNX_LAYER_SYSTEM;

namespace lynx::ecs {
    //Es el ID que tiene la entidad en el motor.
    using Entity = std::uint32_t;
    //const Entity MAX_ENTITIES = lynx::core::options::ECSOptions::GameMaxEntities;//Máximo 100 mil entidades a la vez.
    const Entity NULL_ENTITY = std::numeric_limits<Entity>::max();

    //Es un ID único para cada tipo de componente
    using ComponentType = std::uint8_t;
    const ComponentType MAX_COMPONENTS = 254;
    const ComponentType NULL_COMPONENT = 255;

    //Un bit flag que representa que componentes tiene la entidad.
    using Signature = std::bitset<MAX_COMPONENTS>;

    using UUID = std::uint64_t;

    // Generador de UUIDs seguro para Multi-threading
    static UUID GenerateUUID() {
        static thread_local std::random_device rd;
        static thread_local std::mt19937_64 gen(rd());
        static thread_local std::uniform_int_distribution<uint64_t> dis(1ULL, std::numeric_limits<uint64_t>::max());
        return dis(gen);
    }

    namespace components{
        //Componente que tiene como fin ofrecer metadatos vitales de cada entidad.
        LYNX_COMPONENT
        struct EntityMetadataComponent {
            explicit EntityMetadataComponent(
                TagSystem tag = TagSystem::None,
                LayerSystem layer = LayerSystem::None,
                std::string name = "Entity"
            ) : tags(tag), layers(layer), name(name)
            {}

            LYNX_PROPERTY UUID uuid = GenerateUUID();
            LYNX_PROPERTY std::string name = "Entity"; //Nombre de la entidad

            //Físicas
            LYNX_PROPERTY TagSystem tags = TagSystem::None;
            LYNX_PROPERTY LayerSystem layers = LayerSystem::None;

            //Lifecycle
            LYNX_PROPERTY bool activeStatus = true;

            bool hasTag(TagSystem query) const {
                return (query & this->tags) != TagSystem::None;
            }

            void setTag(TagSystem query){
                this->tags = this->tags | query;
            }

            void removeTag(TagSystem query){
                tags = tags & ~query;
            }

            bool inLayer(LayerSystem query) const {
                return (this->layers & query) != LayerSystem::None;
            }

            void setLayer(LayerSystem query) {
                layers = layers | query;
            }

            void removeLayer(LayerSystem query){
                layers = layers & ~query;
            }

            bool isActive() { return activeStatus; }
            void setActive(bool status) { activeStatus = status; }
            
            void setName(std::string newName) { name = newName; }
            std::string getName() const {return name; }

            TagSystem getTags() const {return tags; }
            LayerSystem getLayers() const {return layers; }
        }; 
    } //namespace components
} // namespace lynx::ecs
