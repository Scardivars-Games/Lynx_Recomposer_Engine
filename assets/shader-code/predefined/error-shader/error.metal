#include <metal_stdlib>

using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
};

struct VertexOut {
    float4 position [[position]];
};

vertex VertexOut error_vertex(uint vertexID [[vertex_id]], device const VertexIn* vertices[[buffer(0)]]){
    VertexOut out;
    out.position = float4(vertices[vertexID].position, 1.0);
    return out;
}

fragment float4 error_fragment(VertexOut in [[stage_in]]){
    return float4(1.0, 0.0, 1.0, 1.0); // Rosado puro
}