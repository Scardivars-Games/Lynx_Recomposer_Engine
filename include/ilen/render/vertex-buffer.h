#include <ilen/maths/vector.h>
#include <ilen/maths/matrix.h>
#include <ilen/graphics/color.h>


namespace ilen::render::buffers {
    //Estructurá básica que el motor usa para almacenar los datos del VertexBuffer una vez cargado los datos de un modelo.
    struct VertexBufferData {
        float4 vertexPosition;
        ilen::graphics::Color vertexColor;
        float2 UV;
    };
}