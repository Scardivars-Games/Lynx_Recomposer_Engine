#pragma once

#include <ilen/headers/enumerations.h>
#include <ilen/graphics/material.h>
#include <ilen/render/gpu-interfaces/vertex-layouts.h>

namespace ilen::render {

    struct IlenRenderPipelineDescriptor {
        std::string pipelineName = "No Name Pipeline";
        uint32_t vertexShaderID = 4;
        uint32_t fragmentShaderID = 4;
        std::string vertexFunctionName = "error_vertex";
        std::string fragmentFunctionName = "error_fragment";
        ILEN_GPU_RENDER_QUEUE_TYPE renderQueue = ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OPAQUE;
        ILEN_VERTEX_LAYOUT_TYPE vertexLayout = ILEN_VERTEX_LAYOUT_TYPE::ILEN_VERTEX_LAYOUT_GLOBAL;

        explicit IlenRenderPipelineDescriptor(const graphics::IlenMaterial& material) {
            pipelineName = material.materialName;
            vertexShaderID = material.vertexShaderID;
            fragmentShaderID = material.fragmentShaderID;
            vertexFunctionName = material.vertexFunctionName;
            fragmentFunctionName = material.fragmentFunctionName;
            renderQueue = material.renderQueue;
        }

        bool operator<(const IlenRenderPipelineDescriptor& other) const {
            return std::tie(vertexShaderID,
                fragmentShaderID,
                vertexFunctionName,
                fragmentFunctionName,
                renderQueue,
                vertexLayout) <
                    std::tie(other.vertexShaderID,
                        other.fragmentShaderID,
                        other.vertexFunctionName,
                        other.fragmentFunctionName,
                        other.renderQueue,
                        other.vertexLayout);
        }
    };

    //Interfaz que abstrae la Render Pipeline de cada API gráfica
    class IGPURenderPipeline {
        public:
            virtual ~IGPURenderPipeline() = default;
            virtual const void* getNativeRenderPipeline() const = 0;
            static void* generateVertexDescriptor(VertexBufferLayout layout);
            virtual void setID(uint16_t id) { m_psoID = id; }
            virtual uint16_t getID() const { return m_psoID; }
            virtual ILEN_GPU_RENDER_QUEUE_TYPE getRenderQueue() const { return m_renderQueue; }
        protected:
            ILEN_GPU_RENDER_QUEUE_TYPE m_renderQueue = ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OPAQUE;
            uint16_t m_psoID = 0;
    };
}
