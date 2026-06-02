#pragma once

#include <ilen/headers/enumerations.h>
#include <ilen/graphics/material.h>
#include <ilen/render/gpu_interfaces/vertex_layouts.h>
#include <ilen/core/bitflags.h>
#include <ilen/render/gpu_interfaces/parallel_command_encoder.h>

namespace ilen::render {
    struct IlenParallelEncoderDescriptor;


    //Esta estructura incluye que valores deben activarse o no a la hora de construir el PSO que esta en uso.
    //De momento solo acepta bools
    struct IlenConstantValuesForPipeline {
        bool actualValue = false;
        uint32_t index = 0;
    };

    struct IlenRenderPipelineDescriptor {
        std::string pipelineName = "No Name Pipeline";
        uint32_t vertexShaderID = 4;
        uint32_t fragmentShaderID = 4;
        std::string vertexFunctionName = "error_vertex";
        std::string fragmentFunctionName = "error_fragment";
        ILEN_GPU_RENDER_QUEUE_TYPE renderQueue = ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OPAQUE;
        ILEN_VERTEX_LAYOUT_TYPE vertexLayout = ILEN_VERTEX_LAYOUT_TYPE::ILEN_VERTEX_LAYOUT_GLOBAL;
        ILEN_GPU_COMPARE_FUNCTION depthCompareFunction = ILEN_GPU_COMPARE_FUNCTION::ILEN_GPU_COMPARE_LESS;

        IlenRenderPipelineOptionsFlags renderPipelineOptionFlags = 0; //Todo activado
        IlenParallelEncoderDescriptor* encoderDescriptor = nullptr; //Para obtener como debe construir el constructor
        std::vector<IlenConstantValuesForPipeline> constantValuesForPipelines;
        std::vector<structs::GlobalUniformIdentifier> psoGlobalUniformIdentifiers = {structs::NOT_DEFINED}; //Necesario para añdir los uniforms globales al shader.

        IlenRenderPipelineDescriptor() = default;
        
        IlenRenderPipelineDescriptor(const graphics::IlenMaterial& material) {
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
            ILEN_GPU_RENDER_QUEUE_TYPE getRenderQueue() const { return m_renderQueue; }
            const IlenRenderPipelineDescriptor getPSODescriptor() const { return descriptor;}
        protected:
            ILEN_GPU_RENDER_QUEUE_TYPE m_renderQueue = ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OPAQUE;
            uint32_t m_psoID = 0;
            IlenRenderPipelineDescriptor descriptor{};
    };
}
