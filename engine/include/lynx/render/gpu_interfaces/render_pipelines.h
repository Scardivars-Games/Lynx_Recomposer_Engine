#pragma once

#include <lynx/headers/enumerations.h>
#include <lynx/graphics/material.h>
#include <lynx/render/gpu_interfaces/vertex_layouts.h>
#include <lynx/core/bitflags.h>
#include <lynx/render/gpu_interfaces/parallel_command_encoder.h>

namespace lynx::render {
    struct LynxParallelEncoderDescriptor;


    //Esta estructura incluye que valores deben activarse o no a la hora de construir el PSO que esta en uso.
    //De momento solo acepta bools
    struct LynxConstantValuesForPipeline {
        bool actualValue = false;
        uint32_t index = 0;
    };

    struct LynxRenderPipelineDescriptor {
        std::string pipelineName = "No Name Pipeline";
        uint32_t vertexShaderID = 4;
        uint32_t fragmentShaderID = 4;
        std::string vertexFunctionName = "error_vertex";
        std::string fragmentFunctionName = "error_fragment";
        LYNX_GPU_RENDER_QUEUE_TYPE renderQueue = LYNX_GPU_RENDER_QUEUE_TYPE::LYNX_GPU_RENDER_QUEUE_OPAQUE;
        LYNX_VERTEX_LAYOUT_TYPE vertexLayout = LYNX_VERTEX_LAYOUT_TYPE::LYNX_VERTEX_LAYOUT_GLOBAL;
        LYNX_GPU_COMPARE_FUNCTION depthCompareFunction = LYNX_GPU_COMPARE_FUNCTION::LYNX_GPU_COMPARE_LESS;

        LynxRenderPipelineOptionsFlags renderPipelineOptionFlags = 0; //Todo activado
        LynxParallelEncoderDescriptor* encoderDescriptor = nullptr; //Para obtener como debe construir el constructor
        std::vector<LynxConstantValuesForPipeline> constantValuesForPipelines;
        std::vector<structs::GlobalUniformIdentifier> psoGlobalUniformIdentifiers = {structs::NOT_DEFINED}; //Necesario para añdir los uniforms globales al shader.

        LynxRenderPipelineDescriptor() = default;
        
        LynxRenderPipelineDescriptor(const graphics::LynxMaterial& material) {
            pipelineName = material.materialName;
            vertexShaderID = material.vertexShaderID;
            fragmentShaderID = material.fragmentShaderID;
            vertexFunctionName = material.vertexFunctionName;
            fragmentFunctionName = material.fragmentFunctionName;
            renderQueue = material.renderQueue;
        }

        bool operator<(const LynxRenderPipelineDescriptor& other) const {
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
            LYNX_GPU_RENDER_QUEUE_TYPE getRenderQueue() const { return m_renderQueue; }
            const LynxRenderPipelineDescriptor getPSODescriptor() const { return descriptor;}
        protected:
            LYNX_GPU_RENDER_QUEUE_TYPE m_renderQueue = LYNX_GPU_RENDER_QUEUE_TYPE::LYNX_GPU_RENDER_QUEUE_OPAQUE;
            uint32_t m_psoID = 0;
            LynxRenderPipelineDescriptor descriptor{};
    };
}
