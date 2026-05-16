#include <metal_stdlib>
#include <ilen/render/render-constants.h> // Tu header agnóstico

using namespace metal;
using namespace ilen::render; // Para structs y slots

struct VertexIn {
    float3 position [[attribute(slots::VertexBufferAttribute_Position)]]; // Attr 0
    float4 color    [[attribute(slots::VertexBufferAttribute_Color)]];    // Attr 1
    float2 uv       [[attribute(slots::VertexBufferAttribute_UV)]];       // Attr 2
    float3 normal   [[attribute(slots::VertexBufferAttribute_Normal)]];   // Attr 3
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
    float2 uv;
};

vertex VertexOut basic_vertex(
    VertexIn in                  [[stage_in]],
    CONSTANT structs::CameraData* camData [[buffer(slots::GlobalBufferSlot_Camera)]],
    CONSTANT structs::InstanceData* instanceData [[buffer(slots::RenderBufferSlot_InstanceModel)]],
    uint instanceID [[instance_id]]
) {
    VertexOut out;
    float4x4 modelMatrix = instanceData[instanceID].modelMatrix;
    float4x4 vpMatrix = camData->currentViewProjectionMatrix;
    out.position = vpMatrix * modelMatrix * float4(in.position, 1.0);
    out.color = in.color;
    out.uv = in.uv;
    return out;
}

fragment float4 basic_fragment(
    VertexOut in [[stage_in]],
    texture2d<float> mainTexture [[texture(0)]],
    sampler textureSampler [[sampler(0)]]
) {
    float4 texColor = mainTexture.sample(textureSampler, in.uv);
    
    if(texColor.a < 0.1) {
        discard_fragment();
    }
    
    return texColor * in.color;
}
