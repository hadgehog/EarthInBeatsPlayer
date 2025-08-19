struct VSOut
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D gTex : register(t0);
SamplerState gSamp : register(s0);

float4 main(VSOut i) : SV_Target
{
    return float4(gTex.Sample(gSamp, i.uv).rgb, 1.0);
}