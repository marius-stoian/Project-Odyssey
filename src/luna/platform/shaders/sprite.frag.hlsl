// Sprite fragment shader (US-230): the texture's colour, its see-through part multiplied by the draw's alpha (SDL_Renderer's alpha modulation).
// A fragment shader reads textures and samplers from space2.
Texture2D<float4> picture : register(t0, space2);
SamplerState nearest : register(s0, space2);

float4 main(float4 position : SV_Position, float2 uv : TEXCOORD0, float4 color : TEXCOORD1) : SV_Target {
    const float4 texel = picture.Sample(nearest, uv);
    return float4(texel.rgb * color.rgb, texel.a * color.a);
}
