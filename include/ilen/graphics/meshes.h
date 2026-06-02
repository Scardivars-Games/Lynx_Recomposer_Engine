#pragma once

#include <vector>
#include <memory>
#include <ilen/render/gpu_interfaces/buffers.h>
#include <ilen/graphics/color.h>
#include <ilen/maths/vector.h>


namespace ilen::render::buffers {
    //Estructurá básica que el motor usa para almacenar los datos del VertexBuffer una vez cargado los datos de un modelo.
    struct alignas(16) VertexBufferData {
        ilen::maths::vector::float4 vertexPosition;
        ilen::graphics::Color vertexColor;
        ilen::maths::vector::float2 UV;
    };
}

namespace ilen::graphics {
    //Indica que una entidad precisa de cargar sus datos a la VRAM. Debe eliminarse una vez se ha hecho la carga.
    struct MeshData {
        std::vector<ilen::render::buffers::VertexBufferData> vertexBufferData; //Array de los vertices del objeto a ser utilizados.
        std::vector<uint32_t> indexBufferData;
    };

    struct MeshGPULink {
        std::shared_ptr<render::IGPUBuffer> vertexBuffer;
        std::shared_ptr<render::IGPUBuffer> indexBuffer;
        uint32_t indexCount = 0;

        //Necesario para hacer batching de vertices, debido a Vulkan, y pudiendose implementar en otras APIs
        uint32_t vertexBufferStartOffset = 0;
        uint32_t indexBufferStartOffset = 0;
    };
}

