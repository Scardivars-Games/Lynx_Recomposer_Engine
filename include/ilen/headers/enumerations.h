#pragma once 

#include <cstdint>

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
        ILEN_GPU_RENDER_QUEUE_OPAQUE = 50, //Objetos completamente opacos
        ILEN_GPU_RENDER_QUEUE_ALPHA_TEST = 100, //Objetos opacos, pero con agujeros
        ILEN_GPU_RENDER_QUEUE_TRANSPARENT = 200, //Objetos con cierto nivel de transparencia
        ILEN_GPU_RENDER_QUEUE_OVERLAY = 255, //UI, y elementos que se deban dibujar por encima de todo.
        //Esto se mapeara a un uint8_t por lo que el valor máximo es 255
    };

    enum class ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE {
        ILEN_GPU_VERTEX_BUFFER_FORMAT_INT,
        ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT,
        ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT2,
        ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT3,
        ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT4,
    };

    enum class ILEN_CAMERA_PROJECTION_TYPE {
        ILEN_CAMERA_PROJECTION_PERSPECTIVE,
        ILEN_CAMERA_PROJECTION_ORTHOGRAPHIC,
    };

    enum class ILEN_GPU_GRAPHICS_API_TYPE {
        //Por convención, mantener nombres normales de las API.
        Auto,
        Metal,
        Vulkan,
        OpenGL,
        DirectX12,
    };

    enum class ILEN_GPU_TEXTURE_TYPE {
        ILEN_GPU_TEXTURE_TYPE_2D,
        ILEN_GPU_TEXTURE_TYPE_3D,
        ILEN_GPU_TEXTURE_TYPE_CUBE_MAP
    };

    enum class ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE {
        ILEN_GPU_TEXTURE_PIXEL_FORMAT_SCREEN,
        ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH,
        ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_STENCIL, //Texture for depth + stencil.
        ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT_HALF, //16 bit - one channel.
        ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT, //32bit - one channel
        ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT2_HALF, //16 bits - two channels
        ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT2, //32 bits - two channels
        ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT3_HALF, //16 bits - three channels
        ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT3, //32 bits - three channels
        ILEN_GPU_TEXTURE_PIXEL_FORMAT_INVALID,
    };

    //Usos que tendra la textura
    enum ILEN_GPU_TEXTURE_USAGE_BIT_FLAGS{
        ILEN_GPU_TEXTURE_USAGE_RENDERTARGET = 1 << 0,
        ILEN_GPU_TEXTURE_USAGE_SHADER_READ = 1 << 1,
        ILEN_GPU_TEXTURE_USAGE_SHADER_WRITE = 1 << 2,
        ILEN_GPU_TEXTURE_USAGE_DEPTHSTENCIL = 1 << 3,
    };

    enum class ILEN_GPU_TEXTURE_PRESENTATION_TYPE {
        ILEN_GPU_TEXTURE_PRESENTATION_PRESENT,//Esto significa que la textura se muestra en pantalla. En metal sería obtener el drawable (CAMetalDrawable).
        ILEN_GPU_TEXTURE_PRESENTATION_NOTPRESENT,
    };

    //memorias en GPU que propiedades deben tener
    enum class ILEN_GPU_MEMORY_PROPERTY_TYPE {
        ILEN_GPU_MEMORY_PROPERTY_PRIVATE, //Visible solo por la GPU
        ILEN_GPU_MEMORY_PROPERTY_SHARED, //Visible tanto por la GPU como la CPU.
    };

    enum class ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE {
        ILEN_GPU_RENDER_PASS_STORE_OPTION_STORE, //Almacena esa textura
        ILEN_GPU_RENDER_PASS_STORE_OPTION_DISCARD, //La elimina
        ILEN_GPU_RENDER_PASS_STORE_OPTION_DONT_CARE, //Me da igual, el driver optimiza
    };

    enum class ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE {
        ILEN_GPU_RENDER_PASS_LOAD_OPTION_LOAD, //Carga el valor de la textura
        ILEN_GPU_RENDER_PASS_LOAD_OPTION_CLEAR, //La limpia a un valor por defecto antes de usarla.
        ILEN_GPU_RENDER_PASS_LOAD_OPTION_DONT_CARE, //Me da igual, el driver optimiza
    };


    //Enum para las opciones gráficas. El int establecido es de 16 bits.
    enum ILEN_GRAPHICS_OPTIONS_BIT_FLAGS {
        ILEN_GRAPHICS_OPTIONS_USE_DEPTH = 1 << 0,
        ILEN_GRAPHICS_OPTIONS_USE_STENCIL = 1 << 1,
        ILEN_GRAPHICS_OPTIONS_USE_MSAA = 1 << 2,
        ILEN_GRAPHICS_OPTIONS_USE_POSTPROCESS = 1 << 10,
        ILEN_GRAPHICS_OPTIONS_USE_MOTION_BLUR = 1 << 11,
        ILEN_GRAPHICS_OPTIONS_USE_BLOOM = 1 << 12,
    };
}