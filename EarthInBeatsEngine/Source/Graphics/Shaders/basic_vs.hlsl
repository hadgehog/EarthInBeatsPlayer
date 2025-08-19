cbuffer CameraCB : register(b0)
{
    float4x4 viewProj;
}

cbuffer ObjectCB : register(b1)
{
    float4x4 model;
}

struct VSInput
{
    float3 pos : POSITION;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD0;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD0;
};

PSInput main(VSInput vin)
{
    PSInput v;
    float4 wpos = mul(float4(vin.pos, 1.0f), model);
    v.pos = mul(wpos, viewProj);
    v.norm = mul(float4(vin.norm, 0.0f), model).xyz;
    v.uv = vin.uv;
    
    return v;
}