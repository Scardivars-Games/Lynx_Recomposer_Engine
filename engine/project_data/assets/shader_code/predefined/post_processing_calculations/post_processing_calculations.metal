#include <metal_stdlib>
#include <lynx/render/render_constants.h> // Tu header agnóstico

using namespace metal;
using namespace lynx::render;

constant bool hasMotionBlur [[function_constant(slots::TextureBufferSlot_VelocityBuffer)]];

struct VertexIn {
    float3 position [[attribute(slots::VertexBufferAttribute_Position)]]; // Attr 0
    float4 color    [[attribute(slots::VertexBufferAttribute_Color)]];    // Attr 1
    float2 uv       [[attribute(slots::VertexBufferAttribute_UV)]];       // Attr 2
    float3 normal   [[attribute(slots::VertexBufferAttribute_Normal)]];   // Attr 3
};

struct VertexOut {
    float4 position [[position]];
    float4 currentClipPos;
    float4 previousClipPos;
};

struct FragmentOut {
    half4 colorAttach [[color(slots::RenderPassAttachment_Color)]]; //Este esta prohibido de escribir para el PSO que administra este shader, pero es obligatorio en Metal
    half2 velocityBuffer [[color(slots::RenderPassAttachment_VelocityBuffer)]];
};

vertex VertexOut post_processing_calculations_vertex(VertexIn in [[stage_in]], uint instanceID [[instance_id]],
                 CONSTANT structs::CameraData* velocityData [[buffer(slots::GlobalBufferSlot_Camera)]],
                 CONSTANT structs::InstanceDataForVelocityBuffer* instanceData [[buffer(slots::RenderBufferSlot_InstanceModel)]]){
    VertexOut out;
    float4x4 actualWorldPos = instanceData[instanceID].actualModelMatrix;
    float4x4 previousWorldPos = instanceData[instanceID].previousModelMatrix;
    out.position = velocityData->currentViewProjectionMatrix * actualWorldPos * float4(in.position, 1.0);
    out.currentClipPos = out.position;
    out.previousClipPos = velocityData->previousViewProjectionMatrix * previousWorldPos * float4(in.position, 1.0);
    return out;
}

fragment FragmentOut post_processing_calculations_fragment(VertexOut in [[stage_in]]){
    FragmentOut out;
    if(hasMotionBlur){
        //NDC coordinates (Normalized Device Coordinates, de -1 a 1)
       float2 currentNDC = in.currentClipPos.xy / in.currentClipPos.w;
       float2 previousNDC = in.previousClipPos.xy / in.previousClipPos.w;

       //Normalizar a espacio de UV (0  a 1)
       float2 currentUV = currentNDC * float2(0.5, -0.5) + 0.5;
       float2 previousUV = previousNDC * float2(0.5, -0.5) + 0.5;

       float2 velocity = currentUV - previousUV;
       out.velocityBuffer = half2(velocity);
    }
    return out;
}
