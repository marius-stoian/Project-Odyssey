// Blit vertex shader (US-230): one triangle that covers the viewport, made from the vertex number (no vertex buffer). The finished virtual screen
// is drawn through it into the window.
struct Output {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

Output main(uint id : SV_VertexID) {
    const float2 corner = float2((id << 1) & 2, id & 2); // (0,0) (2,0) (0,2)
    Output output;
    output.position = float4(corner.x * 2.0 - 1.0, 1.0 - corner.y * 2.0, 0.0, 1.0);
    output.uv = corner;
    return output;
}
