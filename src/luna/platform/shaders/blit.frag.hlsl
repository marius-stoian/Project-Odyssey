// Blit fragment shader (US-230): the virtual screen's pixels, sampled nearest-neighbour so each one becomes a crisp block.
Texture2D<float4> picture : register(t0, space2);
SamplerState nearest : register(s0, space2);

float4 main(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
    return float4(picture.Sample(nearest, uv).rgb, 1.0);
}
