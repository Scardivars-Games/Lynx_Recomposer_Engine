#pragma once

#include <cstdint>
#include <lynx/ecs/types.h>
#include <vector>

namespace lynx::binary::headers {
    struct LynxManifestHeader {
        char magicNumber[20]; //LYNX_MANIFEST -> TSEFINAM_NELI
        uint32_t version;
        uint32_t totalOfNamesToID;
        uint32_t totalOfModels;
        uint32_t totalOfTextures;
        uint32_t totalOfShaders;
        uint32_t totalOfMaterials;
    };

    struct LynxMeshHeader {
        char magicNumber[20]; // LYNX_MESH -> HSEM_NELI
        uint32_t version;
        uint32_t vertexCount;
        uint32_t indexCount;
    };

    struct LynxShaderHeader {
        char magicNumber[20]; //LYNX_SHADER -> REDAHS_NELI
        uint32_t version;
        uint32_t fileNameSize;
        uint32_t fileRouteSize;
        uint32_t shaderNameSize;
    };

    struct LynxMaterialHeader
    {
        char magicNumber[20]; //LYNX_MATERIAL -> LAIRETAM_NELI
        uint32_t version;
        uint32_t materialNameSize;
        uint32_t vertexFunctionSize;
        uint32_t fragmentFunctionSize;
    };
    
    struct alignas(4) LynxAtlasSpriteInfo {
        uint32_t id;
        float uvRect[4]; //[offsetX, offsetY, scaleX, scaleY].
    };

    struct alignas(8) TexturePayloadInfo {
        lynx::enumerations::LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE format;
        uint64_t offset; //En que byte empiezan los datos de esta estructura
        uint64_t dataSize; //Tamaño de los datos de esa textura en el formato.
    };

    struct LynxTextureHeader {
        char magicNumber[20]; //LYNX_TEXTURE -> ERUTXET_NELI
        uint32_t version;
        uint32_t width;
        uint32_t height;
        uint32_t channels;
        uint32_t mipLevels;
        uint32_t payloadCount; //Tamaño del bloque de datos.
        uint32_t atlasSpriteCount; //Cuantos sprites posee este atlas (0 significa que es una textura simple).
    };

    struct LynxSceneHeader {
        char magicNumber[20]; //LYNX_SCENE -> ENECS_NELI
        uint32_t version;
        uint32_t totalEntities;
        uint32_t totalComponentTypes;
    };

    struct SceneComponentBuffer {
        uint32_t componentHash;
        uint32_t entityCount = 0;
        std::vector<lynx::ecs::UUID> entityIDs;
        std::vector<char> rawData;
    };
}