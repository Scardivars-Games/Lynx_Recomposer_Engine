#pragma once

#include <ilen/headers/enumerations.h>
#include <vector>

namespace ilen::render {
    using namespace ilen::enumerations;
    //Esto supuestamente facilitaría a futuro equivaler cada valor en esta lista con el formato que hay que darle a la API gráfica con sus nombres internos mediante algún método
    struct VertexBufferLayout {
        size_t stride; //Tamaño de cada vertice en memoria
        std::vector<ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE> attributes;

        bool operator<(const VertexBufferLayout& other) const {
            if (stride != other.stride ) {
                return stride < other.stride;
            }

            return attributes < other.attributes;
        }
    };

    const static VertexBufferLayout ILEN_GLOBAL_VERTEX_LAYOUT {
        .stride = 40, // sizeof(float(4 bytes)) * 10 = 40 bytes
        .attributes = {
            ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT4, //Position
            ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT4, //Color
            ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT2 //UV Coords
        }
    };
}