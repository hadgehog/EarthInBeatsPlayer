struct PSInput
{
    float4 pos : SV_POSITION;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD0;
};

Texture2D gBaseColor : register(t0);
SamplerState gSample : register(s0);

float4 main(PSInput pin) : SV_Target
{
    float3 lightDir = normalize(float3(0.3f, 0.6f, 0.5f));
    float3 n = normalize(pin.norm);
    float ndl = saturate(dot(n, lightDir));
    float3 base = gBaseColor.Sample(gSample, pin.uv).rgb;
    float3 lit = base * (0.25 + 0.75 * ndl);
    
    return float4(lit, 1.0);
}