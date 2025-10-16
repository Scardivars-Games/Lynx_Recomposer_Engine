#pragma once

#include <ilen/headers/maths.h>
#include <vector>

namespace ilen::render {    

    //Esto supuestamente facilitaría a futuro equivaler cada valor en esta lista con el formato que hay que darle a la API gráfica con sus nombres internos mediante algún método
    enum class VertexBufferFormats {
        Int, Float, Float2, Float3, Float4
    };

    struct VertexBufferLayout {
        size_t stride; //Tamaño de cada vertice en memoria
        std::vector<VertexBufferFormats> attributes;
    };

    struct IlenRenderPipelineDescriptor {
        
    };

    //Interfaz que abstrae la Render Pipeline de cada API gráfica
    class IGPURenderPipeline {

    };
}