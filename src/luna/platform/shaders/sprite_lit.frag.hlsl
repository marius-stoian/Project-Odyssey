// Lit sprite fragment shader (US-240, ADR-021): the sprite's colour times the light that reaches it: an ambient colour plus up to 64 point lights.
// Each light brightens by how close it is (a smooth fall-off to its radius) and by how squarely the sprite's surface faces it, from the sprite's
// normal map (x right, y down, z toward the viewer; a sprite without one is flat, normal 0 0 1). A fragment shader reads textures and samplers
// from space2 and its uniform buffer from space3.
Texture2D<float4> picture : register(t0, space2);
Texture2D<float4> normals : register(t1, space2);
SamplerState nearest : register(s0, space2);
SamplerState nearestForNormals : register(s1, space2);

static const int kMaxLights = 64;

cbuffer Lighting : register(b0, space3) {
    float4 ambient;                // rgb: the ambient colour (already times its strength); w: the number of lights
    float4 lights[kMaxLights * 2]; // per light: (x, y, radius, strength) then (r, g, b, height), in virtual pixels
};

float4 main(float4 position : SV_Position, float2 uv : TEXCOORD0, float4 color : TEXCOORD1, float2 pixel : TEXCOORD2) : SV_Target {
    const float4 texel = picture.Sample(nearest, uv);
    const float3 normal = normalize(normals.Sample(nearestForNormals, uv).xyz * 2.0 - 1.0);
    float3 light = ambient.rgb;
    const int count = min((int)ambient.w, kMaxLights);
    for (int i = 0; i < count; ++i) {
        const float4 a = lights[i * 2];
        const float4 b = lights[i * 2 + 1];
        const float2 toLight = a.xy - pixel;
        const float reach = saturate(1.0 - length(toLight) / a.z);
        const float facing = saturate(dot(normal, normalize(float3(toLight, b.w))));
        light += b.rgb * (a.w * reach * reach * facing);
    }
    // Many lights on one spot add up to more than a sprite can show: past full brightness the light is squeezed toward 1.5 instead of washing the colours out to white.
    const float3 over = max(light - 1.0, 0.0);
    light = min(light, 1.0) + over / (1.0 + over * 2.0);
    return float4(texel.rgb * color.rgb * light, texel.a * color.a);
}
