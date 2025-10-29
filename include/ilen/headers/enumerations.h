#pragma once 

namespace ilen::enumerations {
    enum ILEN_ASSET_TYPE {
        ILEN_ASSET_TYPE_MODEL,
        ILEN_ASSET_TYPE_TEXTURE,
        ILEN_ASSET_TYPE_MATERIAL,
        ILEN_ASSET_TYPE_AUDIO,
        ILEN_ASSET_TYPE_SHADER,
        //No se incluye shader entre los tipos de assets, ya que el motor no le asigna los shaders a las entidades, sino los materiales.
        //Aún así, este objeto si mantiene una lista con los id y direcciones de los shaders para que el material se pueda crear y asignar.
    };

    enum ILEN_FILE_TYPE{
        ILEN_FILE_TYPE_NATIVE_SHADER,
        ILEN_FILE_TYPE_ENGINE_SHADER,
        ILEN_FILE_TYPE_TEXTURE,
        ILEN_FILE_TYPE_MODEL,
        ILEN_FILE_TYPE_MATERIAL,
        ILEN_FILE_TYPE_ASSETS_MANIFEST,
        ILEN_FILE_TYPE_NONE //Relativo a assets en este caso.
    };

    enum ILEN_ANGLES_TYPE {
        ILEN_ANGLE_TYPE_DEGREES,
        ILEN_ANGLE_TYPE_RADIANS
    };

    enum ILEN_WINDOW_PROPERTIES_BIT_FLAGS{
        ILEN_WINDOW_PROPERTY_FULLSCREEN = 0, //Si una pantalla esta en fullscreen, no necesita ninguna opción extra.
        ILEN_WINDOW_PROPERTY_RESIZABLE = 1 << 0,
        ILEN_WINDOW_PROPERTY_CLOSABLE = 1 << 1,
        ILEN_WINDOW_PROPERTY_MAXIMIZABLE = 1 << 2,
        ILEN_WINDOW_PROPERTY_BORDERLESS = 1 << 3,
        ILEN_WINDOW_PROPERTY_WINDOWED = 0 << 3,
    };

    //Tipos de Buffers para la GPU
    enum ILEN_RENDER_GPU_BUFFER_TYPE {
        ILEN_RENDER_GPU_BUFFER_VERTEX,
        ILEN_RENDER_GPU_BUFFER_INDEX,
        ILEN_RENDER_GPU_BUFFER_UNIFORM
    };

    enum class ILEN_VERTEX_LAYOUT_TYPE {
        ILEN_VERTEX_LAYOUT_GLOBAL = 1,
        ILEN_VERTEX_LAYOUT_BONED = 2,
        ILEN_VERTEX_LAYOUT_PARTICLES = 3,
        ILEN_VERTEX_LAYOUT_DEBUG = 4,
        ILEN_VERTEX_LAYOUT_POST_PROCESS = 5,
    };

    //RenrerQueue de los Materiales (Usado para organizar las draw calls, y saber el estado a asignar en el PSO)}
    enum class ILEN_GPU_RENDER_QUEUE_TYPE {
        ILEN_GPU_RENDER_QUEUE_OPAQUE = 1000, //Objetos completamente opacos
        ILEN_GPU_RENDER_QUEUE_ALPHA_TEST = 2000, //Objetos opacos, pero con agujeros
        ILEN_GPU_RENDER_QUEUE_TRANSPARENT = 3000, //Objetos con cierto nivel de transparencia
        ILEN_GPU_RENDER_QUEUE_OVERLAY = 4000, //UI, y elementos que se deban dibujar por encima de todo.
    };

    enum class ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE {
        ILEN_GPU_VERTEX_BUFFER_FORMAT_INT,
        ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT,
        ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT2,
        ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT3,
        ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT4,
    };
}