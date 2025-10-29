#include <metal_stdlib>

using namespace metal;

struct VertexIn {
    float4 position [[attribute(0)]];
    float4 color [[attribute(1)]];
    float2 uv [[attribute(2)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
    float2 uv;
};

vertex VertexOut basic_vertex(uint vertexID [[vertex_id]], device const VertexIn* vertices [[buffer(0)]]) {
    VertexOut out;
    out.position = float4(vertices[vertexID].position);
    out.color = vertices[vertexID].color;
    out.uv = vertices[vertexID].uv;
    return out;
}

fragment float4 basic_fragment(VertexOut in [[stage_in]]) {
    return in.color;
}
