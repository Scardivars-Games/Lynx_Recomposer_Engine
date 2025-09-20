#include <ilen/maths/vector.h>
#include <ilen/maths/matrix.h>

namespace ilen::render::buffers {
    //Estructurá básica que el motor usa para almacenar los datos del VertexBuffer una vez cargado los datos de un modelo.
    struct VertexBuffer {
        float4 vertexPosition;
        float4 vertexColor;
        float2 UV;
    };
}