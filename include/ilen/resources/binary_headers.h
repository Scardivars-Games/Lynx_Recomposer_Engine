#pragma once

#include <cstdint>

namespace ilen::binary::headers {
    struct IlenManifestHeader {
        char magicNumber[20]; //ILEN_MANIFEST -> TSEFINAM_NELI
        uint32_t version;
        uint32_t totalOfNamesToID;
        uint32_t totalOfModels;
        uint32_t totalOfTextures;
        uint32_t totalOfShaders;
        uint32_t totalOfMaterials;
    };

    struct IlenMeshHeader {
        char magicNumber[20]; // ILEN_MESH -> HSEM_NELI
        uint32_t version;
        uint32_t vertexCount;
        uint32_t indexCount;
    };

    struct IlenShaderHeader {
        char magicNumber[20]; //ILEN_SHADER -> REDAHS_NELI
        uint32_t version;
        uint32_t fileNameSize;
        uint32_t fileRouteSize;
        uint32_t shaderNameSize;
    };

    struct IlenMaterialHeader
    {
        char magicNumber[20]; //ILEN_MATERIAL -> LAIRETAM_NELI
        uint32_t version;
        uint32_t materialNameSize;
        uint32_t vertexFunctionSize;
        uint32_t fragmentFunctionSize;
    };
}