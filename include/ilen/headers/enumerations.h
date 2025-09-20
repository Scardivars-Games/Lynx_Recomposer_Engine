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
        ILEN_FILE_TYPE_TEXTURE,
        ILEN_FILE_TYPE_MODEL,
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
}