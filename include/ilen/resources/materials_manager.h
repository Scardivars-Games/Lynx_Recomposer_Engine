#pragma once

#include <string>
#include <unordered_map>
#include <future>
#include <mutex>
#include <ilen/core/lifetime.h>
#include <ilen/graphics/material.h>
#include <ilen/resources/assets_manager.h>
#include <ilen/resources/binary_headers.h>
#include <ilen/job_system/job_system.h>

namespace ilen
{
    struct  EngineContext;
}

namespace ilen::graphics {
    struct IlenMaterial;
}

namespace ilen::resources {
    class AssetsManager;
    using namespace ilen::enumerations;

    template <ILEN_ASSET_TYPE T>
    struct AssetElement;

    using MaterialAsset = AssetElement<ILEN_ASSET_TYPE_MATERIAL>;
}

namespace ilen::resources::materials {
    using namespace ilen::render;
    using namespace ilen::graphics;
    using namespace ilen::job_system;

    using MaterialID = uint32_t; //Solo legibilidad
    using RuntimeMaterialID = uint16_t; //Necesario para el ordenamiento al renderizar de forma eficiente. Solo tiene en cuenta los materiales en memoria.

    //This class is intended to be unique in the game engine. Is loaded using the Service Locator pattern. It handles all the logic to find Materials in the engine. Cause Materials all simple data, at start all of them will be loaded in RAM.
    class MaterialManager {
        public:
            explicit MaterialManager(EngineContext* context); //Constructor
            const IlenMaterial& getMaterial(MaterialID id); //Obtain a material form m_materials using their MaterialID
            RuntimeMaterialID getRuntimeMaterial(MaterialID id) const;
        private:
            void LoadMaterialData(const MaterialAsset& asset); //From a MaterialAsset, we generate an IlenMaterial. Used in the initial precharge of materials, and as a fallback function when the game recibes a solitude of a not registered material, but is available in the manifest
            std::unordered_map<MaterialID, IlenMaterial> m_materials; //Links every valid material with their respective engine ID.
            std::unordered_map<MaterialID, RuntimeMaterialID> m_materialsRuntime; //Return the valid runtime ID for a material using their global ID.

            uint16_t m_nextRuntimeID = 1;

            JobSystem* p_jobSystem; //Pointer to the engine job system for IO operations
            AssetsManager* p_assetsManager; //Pointer to the assets manager of the engine.
            mutable std::recursive_mutex m_mutex; //To make multithreading operations securely.

            //Debug Stuff.
            const char* DEBUG_PREFIX = "MATERIAL MANAGER";
    };
}