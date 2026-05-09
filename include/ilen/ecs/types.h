//Entity-Component system, tipos básicos de cada entidad.
#pragma once
#include <cstdint>
#include <bitset>
#include <limits>
#include <ilen/headers/enumerations.h>

using TagSystem = ilen::enumerations::ILEN_TAG_SYSTEM;
using LayerSystem = ilen::enumerations::ILEN_LAYER_SYSTEM;

namespace ilen::ecs {
    //Es el ID que tiene la entidad en el motor.
    using Entity = std::uint32_t;
    const Entity MAX_ENTITIES = 100000;//Máximo 100 mil entidades a la vez.
    const Entity NULL_ENTITY = std::numeric_limits<Entity>::max();

    //Es un ID único para cada tipo de componente
    using ComponentType = std::uint8_t;
    const ComponentType MAX_COMPONENTS = 254;
    const ComponentType NULL_COMPONENT = 255;

    //Un bit flag que representa que componentes tiene la entidad.
    using Signature = std::bitset<MAX_COMPONENTS>;

    namespace components{
        //Componente que tiene como fin ofrecer metadatos vitales de cada entidad.
        struct EntityMetadataComponent {
            explicit EntityMetadataComponent(
                TagSystem tag = TagSystem::None,
                LayerSystem layer = LayerSystem::None,
                std::string name = "Entity"
            ) : tags(tag), layers(layer), name(name)
            {}

            private:
                std::string name = "Entity"; //Nombre de la entidad

                //Físicas
                TagSystem tags = TagSystem::None;
                LayerSystem layers = LayerSystem::None;

                //Lifecycle
                bool activeStatus = true;
            public:
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
} // namespace ilen::ecs
