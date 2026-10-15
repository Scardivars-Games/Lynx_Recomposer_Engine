#pragma once 

#include <cstdint>

namespace lynx::enumerations {
    enum LYNX_ASSET_TYPE {
        LYNX_ASSET_TYPE_MODEL,
        LYNX_ASSET_TYPE_TEXTURE,
        LYNX_ASSET_TYPE_MATERIAL,
        LYNX_ASSET_TYPE_AUDIO,
        LYNX_ASSET_TYPE_SHADER,
        //No se incluye shader entre los tipos de assets, ya que el motor no le asigna los shaders a las entidades, sino los materiales.
        //Aún así, este objeto si mantiene una lista con los id y direcciones de los shaders para que el material se pueda crear y asignar.
    };

    enum LYNX_FILE_TYPE{
        LYNX_FILE_TYPE_NATIVE_SHADER,
        LYNX_FILE_TYPE_ENGINE_SHADER,
        LYNX_FILE_TYPE_TEXTURE,
        LYNX_FILE_TYPE_MODEL,
        LYNX_FILE_TYPE_MATERIAL,
        LYNX_FILE_TYPE_ASSETS_MANIFEST,
        LYNX_FILE_TYPE_SCENE,
        LYNX_FILE_TYPE_PREFAB,
        LYNX_FILE_TYPE_NONE //Relativo a assets en este caso.
    };

    enum LYNX_ANGLES_TYPE {
        LYNX_ANGLE_TYPE_DEGREES,
        LYNX_ANGLE_TYPE_RADIANS
    };

    enum LYNX_WINDOW_PROPERTIES_BIT_FLAGS{
        LYNX_WINDOW_PROPERTY_FULLSCREEN = 1 << 0, //Si una pantalla esta en fullscreen, no necesita ninguna opción extra.
        LYNX_WINDOW_PROPERTY_RESIZABLE = 1 << 1,
        LYNX_WINDOW_PROPERTY_CLOSABLE = 1 << 2,
        LYNX_WINDOW_PROPERTY_MAXIMIZABLE = 1 << 3,
        LYNX_WINDOW_PROPERTY_BORDERLESS = 1 << 4,
        LYNX_WINDOW_PROPERTY_WINDOWED = 0,
    };

    //Tipos de Buffers para la GPU
    enum LYNX_RENDER_GPU_BUFFER_TYPE {
        LYNX_RENDER_GPU_BUFFER_VERTEX,
        LYNX_RENDER_GPU_BUFFER_INDEX,
        LYNX_RENDER_GPU_BUFFER_UNIFORM
    };

    enum class LYNX_VERTEX_LAYOUT_TYPE {
        LYNX_VERTEX_LAYOUT_GLOBAL = 1,
        LYNX_VERTEX_LAYOUT_BONED = 2,
        LYNX_VERTEX_LAYOUT_PARTICLES = 3,
        LYNX_VERTEX_LAYOUT_DEBUG = 4,
        LYNX_VERTEX_LAYOUT_POST_PROCESS = 5,
        LYNX_VERTEX_LAYOUT_VERTEXLESS = 6, //Renderizar sin ningun descriptor de vertices
    };

    //RenrerQueue de los Materiales (Usado para organizar las draw calls, y saber el estado a asignar en el PSO)}
    enum class LYNX_GPU_RENDER_QUEUE_TYPE {
        LYNX_GPU_RENDER_QUEUE_OPAQUE = 50, //Objetos completamente opacos
        LYNX_GPU_RENDER_QUEUE_ALPHA_TEST = 100, //Objetos opacos, pero con agujeros
        LYNX_GPU_RENDER_QUEUE_TRANSPARENT = 200, //Objetos con cierto nivel de transparencia
        LYNX_GPU_RENDER_QUEUE_OVERLAY = 255, //UI, y elementos que se deban dibujar por encima de todo.
        //Esto se mapeara a un uint8_t por lo que el valor máximo es 255
    };

    enum class LYNX_GPU_VERTEX_BUFFER_FORMATS_TYPE {
        LYNX_GPU_VERTEX_BUFFER_FORMAT_INT,
        LYNX_GPU_VERTEX_BUFFER_FORMAT_FLOAT,
        LYNX_GPU_VERTEX_BUFFER_FORMAT_FLOAT2,
        LYNX_GPU_VERTEX_BUFFER_FORMAT_FLOAT3,
        LYNX_GPU_VERTEX_BUFFER_FORMAT_FLOAT4,
    };

    enum class LYNX_CAMERA_PROJECTION_TYPE: uint32_t {
        LYNX_CAMERA_PROJECTION_PERSPECTIVE = 0,
        LYNX_CAMERA_PROJECTION_ORTHOGRAPHIC = 1,
    };

    enum class LYNX_GPU_GRAPHICS_API_TYPE {
        //Por convención, mantener nombres normales de las API.
        Auto,
        Metal,
        Vulkan,
        OpenGL,
        DirectX12,
    };

    enum class LYNX_GPU_TEXTURE_TYPE {
        LYNX_GPU_TEXTURE_TYPE_2D,
        LYNX_GPU_TEXTURE_TYPE_3D,
        LYNX_GPU_TEXTURE_TYPE_CUBE_MAP
    };

    enum class LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE : uint32_t {
            // ESTADO Y SISTEMA
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_INVALID = 0,
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_SCREEN,         // Se resuelve dinámicamente al formato de la ventana (Ej: BGRA8)

            // COLOR ESTÁNDAR (8-bit Unsigned Normalized) - Ideal para UI y Albedo
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_R8_UNORM,       // 8 bits - 1 canal (R)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_RG8_UNORM,      // 16 bits - 2 canales (R, G)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_RGBA8_UNORM,    // 32 bits - 4 canales (R, G, B, A)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_RGBA8_SRGB,     // 32 bits - 4 canales (Con corrección Gamma para Albedo)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_BGRA8_UNORM,    // 32 bits - Formato nativo en pantallas de Apple/Windows
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_BGRA8_SRGB,

            // HIGH DYNAMIC RANGE (Flotantes de 16-bit / "Half") - HDR y Post-Procesado
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_R16_FLOAT,      // 16 bits - 1 canal (Equivalente a tu FLOAT_HALF)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_RG16_FLOAT,     // 32 bits - 2 canales (Equivalente a tu FLOAT2_HALF)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_RGBA16_FLOAT,   // 64 bits - 4 canales (El estándar de facto para buffers HDR)

            // PRECISIÓN ALTA (Flotantes de 32-bit) - Ideal para G-Buffers y Computación
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_R32_FLOAT,      // 32 bits - 1 canal (Equivalente a tu FLOAT)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_RG32_FLOAT,     // 64 bits - 2 canales (Equivalente a tu FLOAT2)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_RGB32_FLOAT,    // 96 bits - 3 canales (Precaución: A veces emulado por el driver)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_RGBA32_FLOAT,   // 128 bits - 4 canales (Cálculos de físicas/posiciones exactas)

            // PROFUNDIDAD Y STENCIL (Depth & Stencil)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_16_UNORM, // 16 bits de profundidad pura (Para móviles de baja gama)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_32_FLOAT, // 32 bits de profundidad (Estándar moderno en PC/Consolas)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_24_STENCIL_8, // 24 bits profundidad + 8 bits stencil
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_32_FLOAT_STENCIL_8, // 32 bits profundidad + 8 bits stencil

            // COMPRESIÓN DE BLOQUES (Assets de Juego) - Unificados
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_BC7_UNORM,      // Desktop/Intel (Bloques de 16 bytes)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_BC7_SRGB,       // Desktop/Intel (Bloques de 16 bytes, Gamma corregido)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_ASTC_4X4_UNORM, // Apple/Móvil (Bloques de 16 bytes)
            LYNX_GPU_TEXTURE_PIXEL_FORMAT_ASTC_4X4_SRGB   // Apple/Móvil (Bloques de 16 bytes, Gamma corregido)
        };

    //Usos que tendra la textura
    enum LYNX_GPU_TEXTURE_USAGE_BIT_FLAGS{
        LYNX_GPU_TEXTURE_USAGE_RENDERTARGET = 1 << 0,
        LYNX_GPU_TEXTURE_USAGE_SHADER_READ = 1 << 1,
        LYNX_GPU_TEXTURE_USAGE_SHADER_WRITE = 1 << 2,
        LYNX_GPU_TEXTURE_USAGE_DEPTHSTENCIL = 1 << 3,
    };

    enum class LYNX_GPU_TEXTURE_PRESENTATION_TYPE {
        LYNX_GPU_TEXTURE_PRESENTATION_PRESENT,//Esto significa que la textura se muestra en pantalla. En metal sería obtener el drawable (CAMetalDrawable).
        LYNX_GPU_TEXTURE_PRESENTATION_NOTPRESENT,
    };

    //a la GPU le importa un comino esto, al final, no deja de ser un PIxel format
    /*enum class LYNX_GPU_TEXTURE_FORMAT : uint32_t {
        LYNX_GPU_TEXTURE_RGBA8_UNORM = 0,
        LYNX_GPU_TEXTURE_ASTC_4x4_UNORM = 1,
        LYNX_GPU_TEXTURE_BC7_UNORM = 2,
        LYNX_GPU_TEXTURE_UNSUPPORTED = 3, // Esto es para poder regresar error de que no soporta texturas.
    };*/

    //memorias en GPU que propiedades deben tener
    enum class LYNX_GPU_MEMORY_PROPERTY_TYPE {
        LYNX_GPU_MEMORY_PROPERTY_PRIVATE, //Visible solo por la GPU
        LYNX_GPU_MEMORY_PROPERTY_SHARED, //Visible tanto por la GPU como la CPU.
    };

    enum class LYNX_GPU_RENDER_PASS_STORE_OPTION_TYPE {
        LYNX_GPU_RENDER_PASS_STORE_OPTION_STORE, //Almacena esa textura
        LYNX_GPU_RENDER_PASS_STORE_OPTION_DISCARD, //La elimina
        LYNX_GPU_RENDER_PASS_STORE_OPTION_DONT_CARE, //Me da igual, el driver optimiza
    };

    enum class LYNX_GPU_RENDER_PASS_LOAD_OPTION_TYPE {
        LYNX_GPU_RENDER_PASS_LOAD_OPTION_LOAD, //Carga el valor de la textura
        LYNX_GPU_RENDER_PASS_LOAD_OPTION_CLEAR, //La limpia a un valor por defecto antes de usarla.
        LYNX_GPU_RENDER_PASS_LOAD_OPTION_DONT_CARE, //Me da igual, el driver optimiza
    };

    //Enum para los samplers de textura
    enum class LYNX_GPU_SAMPLER_FILTER_TYPE {
        LYNX_GPU_SAMPLER_FILTER_NEAREST,
        LYNX_GPU_SAMPLER_FILTER_LINEAR,
    };

    enum class LYNX_GPU_SAMPLER_MIP_FILTER_TYPE {
        LYNX_GPU_SAMPLER_MIP_FILTER_NOT_MIPPED,
        LYNX_GPU_SAMPLER_MIP_FILTER_NEAREST,
        LYNX_GPU_SAMPLER_MIP_FILTER_LINEAR,
    };

    enum class LYNX_GPU_SAMPLER_ADDRESS_MODE {
        LYNX_GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE,
        LYNX_GPU_SAMPLER_ADDRESS_REPEAT,
        LYNX_GPU_SAMPLER_ADDRESS_MIRRORED_REPEAT,
        LYNX_GPU_SAMPLER_ADDRESS_CLAMP_TO_BORDER_COLOR
    };

    enum class LYNX_GPU_COMPARE_FUNCTION {
        LYNX_GPU_COMPARE_NEVER,
        LYNX_GPU_COMPARE_LESS,
        LYNX_GPU_COMPARE_LESS_EQUAL,
        LYNX_GPU_COMPARE_EQUAL,
        LYNX_GPU_COMPARE_GREATER,
        LYNX_GPU_COMPARE_GREATER_EQUAL,
        LYNX_GPU_COMPARE_ALWAYS
    };

    //Enum para las opciones gráficas. El int establecido es de 16 bits.
    enum LYNX_GRAPHICS_OPTIONS_BIT_FLAGS {
        LYNX_GRAPHICS_OPTIONS_USE_DEPTH = 1 << 0,
        LYNX_GRAPHICS_OPTIONS_USE_STENCIL = 1 << 1,
        LYNX_GRAPHICS_OPTIONS_USE_MSAA = 1 << 2,
        LYNX_GRAPHICS_OPTIONS_USE_POSTPROCESS = 1 << 10,
        LYNX_GRAPHICS_OPTIONS_USE_MOTION_BLUR = 1 << 11,
        LYNX_GRAPHICS_OPTIONS_USE_BLOOM = 1 << 12,
    };

    //Enum para las opciones activas de cierto Pipeline, y así configurarlo correctamente sin violar las validacines de las APIs.
    enum LYNX_RENDER_PIPELINES_OPTIONS_BIT_FLAGS {
        LYNX_RENDER_PIPELINES_OPTIONS_DONT_WRITE_SCREEN_TEXTURE = 1 << 0, //Si es True, el render pipeline NO puede escribir la textura 0 de salida (la que normalmente muestra cosas en pantalla)
        LYNX_RENDER_PIPELINES_OPTIONS_DONT_USES_DEPTH_BUFFER = 1 << 1, //Si es true, la render pipeline NO configurara un espacio para asignar una textura de Depth
        LYNX_RENDER_PIPELINES_OPTIONS_DONT_USES_STENCIL_BUFFER = 1 << 2, //Si es true, la render pipeline NO  configurara un espacio para asignar una textura de Stencil
        LYNX_RENDER_PIPELINE_OPTIONS_DONT_WRITE_DEPTH_BUFFER = 1 << 3, //Necesario para elementos que leen el depth buffer, pero no escriben en él, como objetos transparentes.
        LYNX_RENDER_PIPELINE_OPTIONS_DONT_CULL_ENABLED = 1 << 4, //Se requiere para indicar que este pso no requiere culling.
    };

    //----FÍSICAS----//
    //Tags
    enum class LYNX_TAG_SYSTEM: uint32_t {
        None = 0,

        //Superfecies
        Ground = 1 << 0,
        Wall = 1 << 1,
        Ceiling = 1 << 2,

        //Tipo de superficie
        PlatformOneWay = 1 << 3,
        PlatformMoving = 1 << 4,

        //Zonas básicas
        Water = 1 << 5,
        Hazard = 1 << 6,
        Bouncy = 1 << 7, //Para rebotar

        //Reservaremos algunos bytes para futuros sistemas

        //Personalizables por el juego/usuario
        UserBit0 = 1 << 16,
        UserBit1 = 1 << 17,
        UserBit2 = 1 << 18,
        UserBit3 = 1 << 19,
        UserBit4 = 1 << 20,
        UserBit5 = 1 << 21,
        UserBit6 = 1 << 22,
        UserBit7 = 1 << 23,
        UserBit8 = 1 << 24,
        UserBit9 = 1 << 25,
        UserBit10 = 1 << 26,
        UserBit11 = 1 << 27,
        UserBit12 = 1 << 28,
        UserBit13 = 1 << 29,
        UserBit14 = 1 << 30,
        UserBit15 = 1u << 31,
    };

    inline LYNX_TAG_SYSTEM operator|(LYNX_TAG_SYSTEM a, LYNX_TAG_SYSTEM b){
        return static_cast<LYNX_TAG_SYSTEM>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline LYNX_TAG_SYSTEM operator&(LYNX_TAG_SYSTEM a, LYNX_TAG_SYSTEM b){
        return static_cast<LYNX_TAG_SYSTEM>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline LYNX_TAG_SYSTEM operator~(LYNX_TAG_SYSTEM a){
        return static_cast<LYNX_TAG_SYSTEM>(~static_cast<uint32_t>(a));
    }

    //LYNX_VERTEX_LAYOUT_PARTICLES
    enum class LYNX_LAYER_SYSTEM: uint32_t {
        None = 0,
        Default = 1 << 0,
        Player = 1 << 1,
        Enemy = 1 << 2,
        Environment = 1 << 3,
        UI = 1 << 4,

        //Users
        UserBit0 = 1 << 10,
        UserBit1 = 1 << 11,
        UserBit2 = 1 << 12,
        UserBit3 = 1 << 13,
        UserBit4 = 1 << 14,
        UserBit5 = 1 << 15,
        UserBit6 = 1 << 16,
        UserBit7 = 1 << 17,
        UserBit8 = 1 << 18,
        UserBit9 = 1 << 19,
        UserBit10 = 1 << 20,
    };

    enum class LYNX_COLLIDER_SHAPE_TYPE_2D: uint8_t {
        Box,
        ConvexPolygon
    };

    inline LYNX_LAYER_SYSTEM operator|(LYNX_LAYER_SYSTEM a, LYNX_LAYER_SYSTEM b)
    {
        return static_cast<LYNX_LAYER_SYSTEM>(
            static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
        );
    }

    inline LYNX_LAYER_SYSTEM operator&(LYNX_LAYER_SYSTEM a, LYNX_LAYER_SYSTEM b)
    {
        return static_cast<LYNX_LAYER_SYSTEM>(
            static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
        );
    }

    inline LYNX_LAYER_SYSTEM operator~(LYNX_LAYER_SYSTEM a){
        return static_cast<LYNX_LAYER_SYSTEM>(~static_cast<uint32_t>(a));
    }
}
