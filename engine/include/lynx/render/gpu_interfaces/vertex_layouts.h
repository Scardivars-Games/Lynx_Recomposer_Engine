#pragma once

#include <lynx/headers/enumerations.h>
#include <vector>
#include <lynx/render/render_constants.h>

namespace lynx::render {
    using namespace lynx::enumerations;
    //Esto supuestamente facilitaría a futuro equivaler cada valor en esta lista con el formato que hay que darle a la API gráfica con sus nombres internos mediante algún método
    struct VertexBufferLayout {
        size_t stride; //Tamaño de cada vertice en memoria
        std::vector<LYNX_GPU_VERTEX_BUFFER_FORMATS_TYPE> attributes;
        std::vector<uint32_t> attributeIndex;
        std::vector<size_t> offsets;

        bool operator<(const VertexBufferLayout& other) const {
            if (stride != other.stride ) {
                return stride < other.stride;
            }

            return attributes < other.attributes;
        }
    };

    inline constexpr size_t alignTo(size_t value, size_t alignment){
        return (value + alignment - 1) & ~(alignment - 1);
    }

    inline constexpr size_t getFormatSize(LYNX_GPU_VERTEX_BUFFER_FORMATS_TYPE format){
        using T = LYNX_GPU_VERTEX_BUFFER_FORMATS_TYPE;

        switch (format) {
            case T::LYNX_GPU_VERTEX_BUFFER_FORMAT_FLOAT:
                return 4;
            case T::LYNX_GPU_VERTEX_BUFFER_FORMAT_FLOAT2:
                return 8;
            case T::LYNX_GPU_VERTEX_BUFFER_FORMAT_FLOAT3:
                return 12;
            case T::LYNX_GPU_VERTEX_BUFFER_FORMAT_FLOAT4:
                return 16;
            default:
                return 4;
        }
    }

    inline VertexBufferLayout computeAlignedVertexBuffer(const std::vector<LYNX_GPU_VERTEX_BUFFER_FORMATS_TYPE>& formats, const std::vector<uint32_t>& indices) {
        VertexBufferLayout layout{};
        layout.attributes = formats;
        layout.attributeIndex = indices;
        layout.offsets.resize(indices.size());

        size_t offset = 0;
        size_t maxAlign = 16;

        for (size_t i = 0; i < formats.size(); ++i) {
            size_t size  = getFormatSize(formats[i]);

            offset = alignTo(offset, size);
            layout.offsets[i] = offset;
            offset += size;
        }

        layout.stride = alignTo(offset, maxAlign); // alineamos el stride total
        return layout;
    }

    const static VertexBufferLayout LYNX_GLOBAL_VERTEX_LAYOUT = computeAlignedVertexBuffer({
            LYNX_GPU_VERTEX_BUFFER_FORMATS_TYPE::LYNX_GPU_VERTEX_BUFFER_FORMAT_FLOAT4, //Position
            LYNX_GPU_VERTEX_BUFFER_FORMATS_TYPE::LYNX_GPU_VERTEX_BUFFER_FORMAT_FLOAT4, //Color
            LYNX_GPU_VERTEX_BUFFER_FORMATS_TYPE::LYNX_GPU_VERTEX_BUFFER_FORMAT_FLOAT2 //UV Coords
        },
        {
            slots::VertexBufferAttribute_Position,
            slots::VertexBufferAttribute_Color,
            slots::VertexBufferAttribute_UV,
        });

    const static VertexBufferLayout LYNX_VERTEXLESS_LAYOUT = computeAlignedVertexBuffer({}, {});
}
