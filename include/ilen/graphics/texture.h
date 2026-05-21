#pragma once


#include <string>
#include <ilen/headers/enumerations.h>
#include <fstream>
#include <ilen/maths/vector.h>
//#include <ilen/graphics/color.h>
#include <unordered_map>
#include <vector>
#include <cstdint>
//#include <variant>
#include <ilen/resources/binary_headers.h>
#include <ilen/render/gpu-interfaces/texture.h>
#include <ilen/render/gpu-interfaces/sampler.h>

namespace ilen::graphics {
    // Propiedades que un material puede enviar a un shader.
    // Usamos std::variant para poder almacenar diferentes tipos de datos en un mismo mapa.

    using namespace ilen::enumerations;

    struct IlenTexture {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 0;
        uint32_t mipLevels = 1;
        std::vector<ilen::maths::vector::vec4> loadedAtlasSprites;

        ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE format = ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_INVALID;

        std::vector<uint8_t> pixelData; //Datos comprimidos en el formato dado

        explicit IlenTexture() = default;
        
        explicit IlenTexture(std::ifstream& stream,
            const ilen::binary::headers::IlenTextureHeader& header,
            ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE activeCompressionFormat)
            : width(header.width),
              height(header.height),
              channels(header.channels),
              mipLevels(header.mipLevels),
              format(activeCompressionFormat) {
            
            if(header.payloadCount == 0) return; //No hay datos de textura.
            
            std::vector<ilen::binary::headers::TexturePayloadInfo> payloads(header.payloadCount);
            stream.read(reinterpret_cast<char*>(payloads.data()), sizeof(ilen::binary::headers::TexturePayloadInfo) * header.payloadCount);


            uint64_t targetOffset = 0;
            uint64_t targetSize = 0;
            bool formatFound = false;

            for(const auto& payload : payloads){
                if(payload.format == activeCompressionFormat){
                    targetOffset = payload.offset;
                    targetSize = payload.dataSize;
                    formatFound = true;
                    break;
                }
            }

            //Despues de la estructura del payload viene la estrucutra del atlas sprite
            if(header.atlasSpriteCount > 0) {
                //Es un atlas sprite
                std::vector<ilen::binary::headers::IlenAtlasSpriteInfo> atlasInfo(header.atlasSpriteCount);
                stream.read(reinterpret_cast<char*>(atlasInfo.data()), sizeof(ilen::binary::headers::IlenAtlasSpriteInfo) * header.atlasSpriteCount);

                loadedAtlasSprites.resize(header.atlasSpriteCount); //El id es el indice dentro del sprite map.
                for(const auto& sprite: atlasInfo){
                    loadedAtlasSprites[sprite.id] = ilen::maths::vector::vec4(
                        sprite.uvRect[0], sprite.uvRect[1], sprite.uvRect[2], sprite.uvRect[3]
                    );
                }
            }else {
                //No es un atlas, ocupa todo el espacio.
                loadedAtlasSprites.resize(1);
                loadedAtlasSprites[0] = ilen::maths::vector::vec4(0.0f, 0.0f, 1.0f, 1.0f);
            }

            //Extraer datos si se encuentra la textura
            if(formatFound && targetSize > 0) {
                pixelData.resize(targetSize);
                stream.seekg(targetOffset, std::ios::beg);
                stream.read(reinterpret_cast<char*>(pixelData.data()), targetSize);
            } else {
                this->format = ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_INVALID;
            }
        }
    };

    //Estructura con los datos necesarios para dibujar la textura en la GPU
    struct TextureGPULink {
        std::shared_ptr<render::IGPUTexture> texture;
        std::shared_ptr<render::IGPUSampler> sampler;

        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 0;

        std::vector<ilen::maths::vector::vec4> atlasSprites;
    };

    /*
    class IlenMaterialHelper {
        public:
            static IlenMaterial generateIlenMaterialFromAsset(const ilen::resources::MaterialAsset& asset);

        private:
            static const inline char* DEBUG_PREFIX = "MATERIAL_HELPER";
    };*/
}
