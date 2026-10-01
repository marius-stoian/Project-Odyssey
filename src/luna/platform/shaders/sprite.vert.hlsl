// Sprite vertex shader (US-230): a corner of a textured quad in virtual-screen pixels, turned into a position on the virtual-screen texture.
// SDL_GPU, D3D12 (DXIL, shader model 6.0): the uniform buffer is register b0 of space1 for a vertex shader, vertex inputs are TEXCOORD<location>.
cbuffer Frame : register(b0, space1) {
    float2 virtualSize; // the virtual screen in pixels (960 x 540)
};

struct Input {
    float2 position : TEXCOORD0; // virtual pixels, y down
    float2 uv : TEXCOORD1;       // 0..1 in the texture
    float4 color : TEXCOORD2;    // multiplies the texture: (1, 1, 1, alpha)
};

struct Output {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
    float2 pixel : TEXCOORD2; // the same corner in virtual pixels: the lit shader measures distances to lights with it (US-240)
};

Output main(Input input) {
    Output output;
    // Pixels to clip space: x from -1 to 1 left to right, y from 1 to -1 top to bottom.
    output.position = float4(input.position.x / virtualSize.x * 2.0 - 1.0, 1.0 - input.position.y / virtualSize.y * 2.0, 0.0, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    output.pixel = input.position;
    return output;
}
