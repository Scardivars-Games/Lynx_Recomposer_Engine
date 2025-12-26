//Estructuras
struct VertexIn {
    float3 position: POSITION;
    float4 color: COLOR0;
    float2 uv: TEXCOORD0;
    float3 normal: NORMAL;
};

struct VertexOut {
    float4 position: SV_Position;
    float4 color: COLOR0;
    float2 uv: TEXCOORD0;
};

[[vk::binding(5,0)]]
cbuffer CameraData {
    float4x4 currentViewProjectionMatrix;
    float4x4 previousViewProjectionMatrix;
};

struct InstanceData {
    float4x4 modelMatrix;
};

[[vk::binding(1,0)]]
StructuredBuffer<InstanceData> instanceBuffer;

//Vertex Shader
[shader("vertex")]
VertexOut basic_vertex(VertexIn in, uint instanceID: SV_InstanceID){
    VertexOut out;
    
    float4x4 model = instanceBuffer[instanceID].modelMatrix;
    float4x4 vp = currentViewProjectionMatrix;

    out.position = mul(vp, mul(model, float4(in.position, 1.0)));
    out.color = in.color;
    out.uv = in.uv;

    return out;
}

//Fragment shader
[shader("fragment")]
float4 basic_fragment(VertexOut in): SV_Target{
    return in.color;
}