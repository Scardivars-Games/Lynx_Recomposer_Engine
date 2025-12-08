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

vertex VertexOut post_processing_vertex(VertexIn in [[stage_in]], uint instanceID [[instance_id]]) {
    VertexOut out;
    out.position = float4(in.position, 1.0);
    out.uv = in.uv;
    return out;
}

fragment float4 post_processing_fragment(VertexOut in [[stage_in]], texture2d<float> colorTexture [[texture(slots::TextureBufferSlot_Color)]],
    texture2d<half> velocityBuffer [[texture(slots::TextureBufferSlot_VelocityBuffer)]]){
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear, address::clamp_to_edge);
    float4 color = colorTexture.sample(textureSampler, in.uv);

    if(!hasMotionBlur){
        return color;
    }

    float4 finalColor = color;

    //Codigo para el motion Blur.
    //color = velocityBuffer.sample(textureSampler, in.uv);
    half4 velocityH = velocityBuffer.sample(textureSampler, in.uv);
    float2 velocity = float2(velocityH.xy);
    velocity.y = -velocity.y;
    float velocityScale = 1.5;
    velocity *= velocityScale;

    //Amount de blur
    float blurAmount = length(velocity);

    const int MAX_SAMPLES = 16;
    int sampleCount = int(clamp(blurAmount * 1000.0f, 1.0f, float(MAX_SAMPLES)));

    if(sampleCount <= 1) {
        return finalColor;
    }

    float4 blurredColor = float4(0.0);

    for(int i = 0; i < sampleCount; i++){
        float t = float(i) / float(sampleCount - 1);
        float2 offsetUV = in.uv - velocity * t;
        blurredColor += colorTexture.sample(textureSampler, offsetUV);
    }

    blurredColor /= float(sampleCount);

    finalColor = blurredColor;
    return finalColor;
}
