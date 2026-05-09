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
    float2 uv;
};

constant bool hasMotionBlur [[function_constant(slots::TextureBufferSlot_VelocityBuffer)]];

vertex VertexOut post_processing_vertex(uint vertexID [[vertex_id]]) {
    VertexOut out;
    //out.position = float4(in.position, 1.0);
    //out.uv = in.uv;
    out.uv = float2((vertexID << 1) & 2, vertexID & 2);

    out.position = float4(out.uv * 2.0 - 1.0, 0.0, 1.0);
    out.position.y = -out.position.y;
    return out;
}

fragment float4 post_processing_fragment(VertexOut in [[stage_in]],
    texture2d<float> colorTexture [[texture(slots::TextureBufferSlot_Color)]],
    texture2d<half> velocityBuffer [[texture(slots::TextureBufferSlot_VelocityBuffer)]]) {

    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear, address::clamp_to_edge);
    float4 color = colorTexture.sample(textureSampler, in.uv);

    if (!hasMotionBlur) {
        return color;
    }

    half4 velocityH = velocityBuffer.sample(textureSampler, in.uv);
    float2 velocity = float2(velocityH.xy);
    //velocity.y = -velocity.y;

    float velocityScale = 1.5;
    velocity *= velocityScale;

    float blurAmount = length(velocity);

    // Sin blur si el movimiento es insignificante
    if (blurAmount < 0.0005) {
        return color;
    }

    const int MAX_SAMPLES = 16;
    // Escala razonable: blurAmount ~0.01 → ~10 samples
    int sampleCount = int(clamp(blurAmount * 500.0, 2.0, float(MAX_SAMPLES)));

    float4 blurredColor = float4(0.0);

    for (int i = 0; i < sampleCount; i++) {
        // Centrado: t va de -0.5 a 0.5
        float t = float(i) / float(sampleCount - 1) - 0.5;
        float2 offsetUV = in.uv + velocity * t;
        blurredColor += colorTexture.sample(textureSampler, offsetUV);
    }

    return blurredColor / float(sampleCount);
}
