// The SDL_GPU backend (US-230, ADR-021): draws the game through the graphics card with shaders. See docs/plans/M8b-renderer-design.md section 3.
//
// A frame is recorded on the CPU while the game draws (every drawTexture adds a textured quad to a list), then replayed on the GPU in present():
//   1. the list of quads goes up to the card in one buffer;
//   2. a first pass draws them, in the order they were made, onto the "virtual screen" (a texture the size of the virtual screen);
//   3. a second pass draws that texture into the shared presentation area with nearest-neighbour sampling.
#include "luna/platform/backend.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstring>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>

#if LUNA_GPU
#include "blit.frag.h"
#include "blit.vert.h"
#include "sprite.frag.h"
#include "sprite.vert.h"
#endif

namespace luna::platform {

#if LUNA_GPU

namespace {

[[noreturn]] void fail(const std::string& what) {
    throw std::runtime_error(what + ": " + SDL_GetError());
}

// One corner of a quad: where (virtual pixels), which part of the texture (0..1) and the colour that multiplies it (1, 1, 1, alpha).
struct Vertex {
    float x = 0.0F;
    float y = 0.0F;
    float u = 0.0F;
    float v = 0.0F;
    float r = 1.0F;
    float g = 1.0F;
    float b = 1.0F;
    float a = 1.0F;
};

// Consecutive quads of one texture and one blend mode are drawn with one call.
struct Batch {
    int texture = 0;
    bool additive = false;
    Uint32 first = 0;
    Uint32 count = 0;
};

struct GpuTexture {
    SDL_GPUTexture* texture = nullptr;
    int width = 0;
    int height = 0;
};

constexpr SDL_GPUTextureFormat kPictureFormat = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

class GpuBackend final : public RenderBackend {
public:
    GpuBackend(SDL_Window* window, int virtualWidth, int virtualHeight, odysseus::core::ScalingMode scaling) : window_(window), virtualWidth_(virtualWidth), virtualHeight_(virtualHeight), scaling_(scaling) {
        try {
            init();
        } catch (...) {
            release(); // a constructor that throws does not run its destructor
            throw;
        }
    }

    ~GpuBackend() override { release(); }

    GpuBackend(const GpuBackend&) = delete;
    GpuBackend& operator=(const GpuBackend&) = delete;

    std::string name() const override { return std::string("gpu (") + driver_ + ")"; }
    bool vsyncEnabled() const override { return true; } // the swapchain presents with VSync
    void setGpuTiming(bool on) override { timing_ = on; }
    double gpuMilliseconds() const override { return timing_ ? gpuMilliseconds_ : -1.0; }
    void setScalingMode(odysseus::core::ScalingMode mode) override { scaling_ = mode; }

    void clear(int red, int green, int blue) override {
        vertices_.clear();
        batches_.clear();
        clearColor_ = {static_cast<float>(red) / 255.0F, static_cast<float>(green) / 255.0F, static_cast<float>(blue) / 255.0F, 1.0F};
    }

    int createTexture(int width, int height, const std::uint8_t* rgba) override {
        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = kPictureFormat;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = static_cast<Uint32>(width);
        info.height = static_cast<Uint32>(height);
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;
        SDL_GPUTexture* texture = SDL_CreateGPUTexture(device_, &info);
        if (texture == nullptr) {
            fail("Cannot create a texture");
        }
        textures_.push_back({texture, width, height});

        // Up through a transfer buffer: the pixels are copied into memory both the CPU and the card can reach, then the card copies them into the texture.
        const Uint32 bytes = static_cast<Uint32>(width) * static_cast<Uint32>(height) * 4U;
        SDL_GPUTransferBufferCreateInfo transferInfo{};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = bytes;
        SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device_, &transferInfo);
        if (transfer == nullptr) {
            fail("Cannot create a transfer buffer");
        }
        void* mapped = SDL_MapGPUTransferBuffer(device_, transfer, false);
        if (mapped == nullptr) {
            SDL_ReleaseGPUTransferBuffer(device_, transfer);
            fail("Cannot map a transfer buffer");
        }
        std::memcpy(mapped, rgba, bytes);
        SDL_UnmapGPUTransferBuffer(device_, transfer);
        SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device_);
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(commands);
        SDL_GPUTextureTransferInfo from{};
        from.transfer_buffer = transfer;
        from.pixels_per_row = static_cast<Uint32>(width);
        from.rows_per_layer = static_cast<Uint32>(height);
        SDL_GPUTextureRegion to{};
        to.texture = texture;
        to.w = static_cast<Uint32>(width);
        to.h = static_cast<Uint32>(height);
        to.d = 1;
        SDL_UploadToGPUTexture(copy, &from, &to, false);
        SDL_EndGPUCopyPass(copy);
        SDL_SubmitGPUCommandBuffer(commands);
        SDL_ReleaseGPUTransferBuffer(device_, transfer); // released once the card has used it
        return static_cast<int>(textures_.size()) - 1;
    }

    void drawTexture(int texture, const odysseus::core::Rect& source, const odysseus::core::Rect& destination, std::uint8_t alpha, bool additive) override {
        const GpuTexture& picture = textures_.at(static_cast<std::size_t>(texture));
        if (destination.width <= 0 || destination.height <= 0 || source.width <= 0 || source.height <= 0 || alpha == 0) {
            return;
        }
        const float x0 = static_cast<float>(destination.x);
        const float y0 = static_cast<float>(destination.y);
        const float x1 = static_cast<float>(destination.x + destination.width);
        const float y1 = static_cast<float>(destination.y + destination.height);
        const float u0 = (static_cast<float>(source.x) + kTexelNudge) / static_cast<float>(picture.width);
        const float v0 = (static_cast<float>(source.y) + kTexelNudge) / static_cast<float>(picture.height);
        const float u1 = (static_cast<float>(source.x + source.width) + kTexelNudge) / static_cast<float>(picture.width);
        const float v1 = (static_cast<float>(source.y + source.height) + kTexelNudge) / static_cast<float>(picture.height);
        const float a = static_cast<float>(alpha) / 255.0F;
        const auto first = static_cast<Uint32>(vertices_.size());
        vertices_.push_back({x0, y0, u0, v0, 1.0F, 1.0F, 1.0F, a});
        vertices_.push_back({x1, y0, u1, v0, 1.0F, 1.0F, 1.0F, a});
        vertices_.push_back({x0, y1, u0, v1, 1.0F, 1.0F, 1.0F, a});
        vertices_.push_back({x1, y0, u1, v0, 1.0F, 1.0F, 1.0F, a});
        vertices_.push_back({x1, y1, u1, v1, 1.0F, 1.0F, 1.0F, a});
        vertices_.push_back({x0, y1, u0, v1, 1.0F, 1.0F, 1.0F, a});
        if (!batches_.empty() && batches_.back().texture == texture && batches_.back().additive == additive) {
            batches_.back().count += 6;
        } else {
            batches_.push_back({texture, additive, first, 6});
        }
    }

    bool timing_ = false;
    double gpuMilliseconds_ = 0.0;

    odysseus::core::Rect presentationRect() const override {
        const odysseus::core::Rect size = outputRect();
        return odysseus::core::presentationArea(size.width, size.height, virtualWidth_, virtualHeight_, scaling_);
    }

    odysseus::core::Rect outputRect() const override {
        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(window_, &width, &height);
        return {0, 0, width, height};
    }

    void present() override {
        SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device_);
        if (commands == nullptr) {
            fail("Cannot start a frame");
        }
        recordScene(commands);
        if (timing_) {
            // The scene (every sprite of the frame, drawn into the virtual screen) is sent on its own and waited for, so the time is
            // the card's work and not the wait for the monitor's refresh that the window's picture brings (US-234).
            // The card first finishes the last frame (its picture waits for the monitor), so that only this frame's work is timed.
            SDL_WaitForGPUIdle(device_);
            const std::uint64_t started = SDL_GetPerformanceCounter();
            SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
            if (fence != nullptr) {
                SDL_WaitForGPUFences(device_, true, &fence, 1);
                SDL_ReleaseGPUFence(device_, fence);
            }
            gpuMilliseconds_ = 1000.0 * static_cast<double>(SDL_GetPerformanceCounter() - started) / static_cast<double>(SDL_GetPerformanceFrequency());
            commands = SDL_AcquireGPUCommandBuffer(device_);
            if (commands == nullptr) {
                fail("Cannot start a frame");
            }
        }
        SDL_GPUTexture* swapchain = nullptr;
        Uint32 width = 0;
        Uint32 height = 0;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, window_, &swapchain, &width, &height)) {
            SDL_CancelGPUCommandBuffer(commands);
            fail("Cannot get the window's picture");
        }
        if (swapchain != nullptr) { // null while the window is minimized: nothing to show
            recordBlit(commands, swapchain, static_cast<int>(width), static_cast<int>(height), blitToWindow_);
        }
        SDL_SubmitGPUCommandBuffer(commands);
        vertices_.clear();
        batches_.clear();
    }
    Pixels readPixels() override {
        // The frame so far is drawn once more into a picture the size of the window, then copied back. Slow; for tests and screenshots.
        const odysseus::core::Rect size = outputRect();
        const auto width = static_cast<Uint32>(size.width);
        const auto height = static_cast<Uint32>(size.height);
        if (blitToPicture_ == nullptr) {
            blitToPicture_ = makeBlitPipeline(kPictureFormat);
        }
        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = kPictureFormat;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = width;
        info.height = height;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;
        SDL_GPUTexture* capture = SDL_CreateGPUTexture(device_, &info);
        SDL_GPUTransferBufferCreateInfo transferInfo{};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
        transferInfo.size = width * height * 4U;
        SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device_, &transferInfo);
        if (capture == nullptr || transfer == nullptr) {
            if (capture != nullptr) SDL_ReleaseGPUTexture(device_, capture);
            if (transfer != nullptr) SDL_ReleaseGPUTransferBuffer(device_, transfer);
            fail("Cannot prepare to read the screen");
        }

        SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device_);
        recordScene(commands);
        recordBlit(commands, capture, size.width, size.height, blitToPicture_);
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(commands);
        SDL_GPUTextureRegion from{};
        from.texture = capture;
        from.w = width;
        from.h = height;
        from.d = 1;
        SDL_GPUTextureTransferInfo to{};
        to.transfer_buffer = transfer;
        to.pixels_per_row = width;
        to.rows_per_layer = height;
        SDL_DownloadFromGPUTexture(copy, &from, &to);
        SDL_EndGPUCopyPass(copy);
        SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
        SDL_WaitForGPUFences(device_, true, &fence, 1);
        SDL_ReleaseGPUFence(device_, fence);

        Pixels pixels;
        pixels.width = size.width;
        pixels.height = size.height;
        pixels.rgba.resize(static_cast<std::size_t>(width) * height * 4U);
        const void* mapped = SDL_MapGPUTransferBuffer(device_, transfer, false);
        if (mapped != nullptr) {
            std::memcpy(pixels.rgba.data(), mapped, pixels.rgba.size());
            SDL_UnmapGPUTransferBuffer(device_, transfer);
        }
        SDL_ReleaseGPUTransferBuffer(device_, transfer);
        SDL_ReleaseGPUTexture(device_, capture);
        if (mapped == nullptr) {
            fail("Cannot read the screen");
        }
        return pixels;
    }

private:
    void init() {
        const bool debug = SDL_getenv("LUNA_GPU_DEBUG") != nullptr; // the graphics debug layer, when asked for and installed
        device_ = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL, debug, nullptr);
        if (device_ == nullptr) {
            fail("Cannot create the GPU device");
        }
        driver_ = SDL_GetGPUDeviceDriver(device_);
        if (!SDL_ClaimWindowForGPUDevice(device_, window_)) {
            fail("Cannot give the window to the GPU");
        }
        windowClaimed_ = true;
        if (!SDL_SetGPUSwapchainParameters(device_, window_, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC)) {
            fail("Cannot set up presenting with VSync");
        }
        windowFormat_ = SDL_GetGPUSwapchainTextureFormat(device_, window_);

        // The virtual screen: everything is drawn here first, then enlarged into the window.
        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = kPictureFormat;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = static_cast<Uint32>(virtualWidth_);
        info.height = static_cast<Uint32>(virtualHeight_);
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;
        virtualScreen_ = SDL_CreateGPUTexture(device_, &info);
        if (virtualScreen_ == nullptr) {
            fail("Cannot create the virtual screen");
        }

        // Nearest-neighbour, never past the edge: pixels stay square and crisp.
        SDL_GPUSamplerCreateInfo samplerInfo{};
        samplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
        samplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
        samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampler_ = SDL_CreateGPUSampler(device_, &samplerInfo);
        if (sampler_ == nullptr) {
            fail("Cannot create the sampler");
        }

        spriteVertex_ = makeShader(g_sprite_vert, sizeof(g_sprite_vert), SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
        spriteFragment_ = makeShader(g_sprite_frag, sizeof(g_sprite_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);
        blitVertex_ = makeShader(g_blit_vert, sizeof(g_blit_vert), SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
        blitFragment_ = makeShader(g_blit_frag, sizeof(g_blit_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);
        spriteNormal_ = makeSpritePipeline(false);
        spriteAdd_ = makeSpritePipeline(true);
        blitToWindow_ = makeBlitPipeline(windowFormat_);
    }

    void release() {
        if (device_ == nullptr) {
            return;
        }
        SDL_WaitForGPUIdle(device_); // nothing may still be in flight when its pieces go
        for (const GpuTexture& texture : textures_) SDL_ReleaseGPUTexture(device_, texture.texture);
        textures_.clear();
        if (vertexBuffer_ != nullptr) SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
        if (vertexTransfer_ != nullptr) SDL_ReleaseGPUTransferBuffer(device_, vertexTransfer_);
        for (SDL_GPUGraphicsPipeline* pipeline : {spriteNormal_, spriteAdd_, blitToWindow_, blitToPicture_}) {
            if (pipeline != nullptr) SDL_ReleaseGPUGraphicsPipeline(device_, pipeline);
        }
        for (SDL_GPUShader* shader : {spriteVertex_, spriteFragment_, blitVertex_, blitFragment_}) {
            if (shader != nullptr) SDL_ReleaseGPUShader(device_, shader);
        }
        if (sampler_ != nullptr) SDL_ReleaseGPUSampler(device_, sampler_);
        if (virtualScreen_ != nullptr) SDL_ReleaseGPUTexture(device_, virtualScreen_);
        if (windowClaimed_) SDL_ReleaseWindowFromGPUDevice(device_, window_);
        SDL_DestroyGPUDevice(device_);
        device_ = nullptr;
    }

    SDL_GPUShader* makeShader(const unsigned char* code, std::size_t size, SDL_GPUShaderStage stage, Uint32 samplers, Uint32 uniformBuffers) {
        SDL_GPUShaderCreateInfo info{};
        info.code = code;
        info.code_size = size;
        info.entrypoint = "main";
        info.format = SDL_GPU_SHADERFORMAT_DXIL;
        info.stage = stage;
        info.num_samplers = samplers;
        info.num_uniform_buffers = uniformBuffers;
        SDL_GPUShader* shader = SDL_CreateGPUShader(device_, &info);
        if (shader == nullptr) {
            fail("A shader does not load");
        }
        return shader;
    }

    // Quads of the sprite shader: normal blending (the picture's see-through parts show what is below) or additive (adds light).
    SDL_GPUGraphicsPipeline* makeSpritePipeline(bool additive) {
        SDL_GPUVertexBufferDescription buffer{};
        buffer.slot = 0;
        buffer.pitch = sizeof(Vertex);
        buffer.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        SDL_GPUVertexAttribute attributes[3]{};
        attributes[0] = {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Vertex, x)};
        attributes[1] = {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Vertex, u)};
        attributes[2] = {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(Vertex, r)};

        SDL_GPUColorTargetDescription target{};
        target.format = kPictureFormat;
        target.blend_state.enable_blend = true;
        target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        target.blend_state.dst_color_blendfactor = additive ? SDL_GPU_BLENDFACTOR_ONE : SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        target.blend_state.src_alpha_blendfactor = additive ? SDL_GPU_BLENDFACTOR_ZERO : SDL_GPU_BLENDFACTOR_ONE;
        target.blend_state.dst_alpha_blendfactor = additive ? SDL_GPU_BLENDFACTOR_ONE : SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

        SDL_GPUGraphicsPipelineCreateInfo info{};
        info.vertex_shader = spriteVertex_;
        info.fragment_shader = spriteFragment_;
        info.vertex_input_state.vertex_buffer_descriptions = &buffer;
        info.vertex_input_state.num_vertex_buffers = 1;
        info.vertex_input_state.vertex_attributes = attributes;
        info.vertex_input_state.num_vertex_attributes = 3;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
        info.target_info.color_target_descriptions = &target;
        info.target_info.num_color_targets = 1;
        SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device_, &info);
        if (pipeline == nullptr) {
            fail("Cannot create the sprite pipeline");
        }
        return pipeline;
    }

    // The last step: the virtual screen into a picture of the given format (the window's, or RGBA for a screenshot). No blending.
    SDL_GPUGraphicsPipeline* makeBlitPipeline(SDL_GPUTextureFormat format) {
        SDL_GPUColorTargetDescription target{};
        target.format = format;
        SDL_GPUGraphicsPipelineCreateInfo info{};
        info.vertex_shader = blitVertex_;
        info.fragment_shader = blitFragment_;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
        info.target_info.color_target_descriptions = &target;
        info.target_info.num_color_targets = 1;
        SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device_, &info);
        if (pipeline == nullptr) {
            fail("Cannot create the picture pipeline");
        }
        return pipeline;
    }

    // Steps 1 and 2: the quads up to the card, then drawn onto the virtual screen in the order they were made.
    void recordScene(SDL_GPUCommandBuffer* commands) {
        const Uint32 bytes = static_cast<Uint32>(vertices_.size() * sizeof(Vertex));
        if (bytes > 0) {
            ensureVertexCapacity(bytes);
            void* mapped = SDL_MapGPUTransferBuffer(device_, vertexTransfer_, true);
            if (mapped == nullptr) {
                fail("Cannot map the vertex transfer buffer");
            }
            std::memcpy(mapped, vertices_.data(), bytes);
            SDL_UnmapGPUTransferBuffer(device_, vertexTransfer_);
            SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(commands);
            SDL_GPUTransferBufferLocation from{vertexTransfer_, 0};
            SDL_GPUBufferRegion to{vertexBuffer_, 0, bytes};
            SDL_UploadToGPUBuffer(copy, &from, &to, true);
            SDL_EndGPUCopyPass(copy);
        }

        SDL_GPUColorTargetInfo target{};
        target.texture = virtualScreen_;
        target.clear_color = clearColor_;
        target.load_op = SDL_GPU_LOADOP_CLEAR;
        target.store_op = SDL_GPU_STOREOP_STORE;
        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commands, &target, 1, nullptr);
        if (bytes > 0) {
            const float size[2] = {static_cast<float>(virtualWidth_), static_cast<float>(virtualHeight_)};
            SDL_PushGPUVertexUniformData(commands, 0, size, sizeof(size));
            SDL_GPUBufferBinding binding{vertexBuffer_, 0};
            SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
            bool boundAdditive = false;
            bool pipelineBound = false;
            for (const Batch& batch : batches_) {
                if (!pipelineBound || batch.additive != boundAdditive) {
                    SDL_BindGPUGraphicsPipeline(pass, batch.additive ? spriteAdd_ : spriteNormal_);
                    boundAdditive = batch.additive;
                    pipelineBound = true;
                }
                const SDL_GPUTextureSamplerBinding picture{textures_[static_cast<std::size_t>(batch.texture)].texture, sampler_};
                SDL_BindGPUFragmentSamplers(pass, 0, &picture, 1);
                SDL_DrawGPUPrimitives(pass, batch.count, 1, batch.first, 0);
            }
        }
        SDL_EndGPURenderPass(pass);
    }

    // Step 3: the virtual screen into the selected area, centred with black outside it.
    void recordBlit(SDL_GPUCommandBuffer* commands, SDL_GPUTexture* destination, int width, int height, SDL_GPUGraphicsPipeline* pipeline) {
        SDL_GPUColorTargetInfo target{};
        target.texture = destination;
        target.clear_color = {0.0F, 0.0F, 0.0F, 1.0F};
        target.load_op = SDL_GPU_LOADOP_CLEAR;
        target.store_op = SDL_GPU_STOREOP_STORE;
        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commands, &target, 1, nullptr);
        const odysseus::core::Rect area = odysseus::core::presentationArea(width, height, virtualWidth_, virtualHeight_, scaling_);
        const SDL_GPUViewport viewport{static_cast<float>(area.x), static_cast<float>(area.y), static_cast<float>(area.width), static_cast<float>(area.height), 0.0F, 1.0F};
        const SDL_Rect scissor{area.x, area.y, area.width, area.height};
        SDL_BindGPUGraphicsPipeline(pass, pipeline);
        SDL_SetGPUViewport(pass, &viewport);
        SDL_SetGPUScissor(pass, &scissor);
        const SDL_GPUTextureSamplerBinding picture{virtualScreen_, sampler_};
        SDL_BindGPUFragmentSamplers(pass, 0, &picture, 1);
        SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
        SDL_EndGPURenderPass(pass);
    }

    // The vertex buffer on the card and the transfer buffer that feeds it grow together, to twice what a frame needs.
    void ensureVertexCapacity(Uint32 bytes) {
        if (bytes <= vertexCapacity_) {
            return;
        }
        if (vertexBuffer_ != nullptr) SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
        if (vertexTransfer_ != nullptr) SDL_ReleaseGPUTransferBuffer(device_, vertexTransfer_);
        vertexCapacity_ = std::max<Uint32>(bytes * 2U, 64U * 1024U);
        SDL_GPUBufferCreateInfo bufferInfo{};
        bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        bufferInfo.size = vertexCapacity_;
        vertexBuffer_ = SDL_CreateGPUBuffer(device_, &bufferInfo);
        SDL_GPUTransferBufferCreateInfo transferInfo{};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = vertexCapacity_;
        vertexTransfer_ = SDL_CreateGPUTransferBuffer(device_, &transferInfo);
        if (vertexBuffer_ == nullptr || vertexTransfer_ == nullptr) {
            fail("Cannot grow the vertex buffer");
        }
    }

    SDL_Window* window_ = nullptr; // owned by the Window, which outlives its backend
    int virtualWidth_ = 0;
    int virtualHeight_ = 0;
    odysseus::core::ScalingMode scaling_ = odysseus::core::ScalingMode::Whole;
    SDL_GPUDevice* device_ = nullptr;
    const char* driver_ = "";
    bool windowClaimed_ = false;
    SDL_GPUTextureFormat windowFormat_ = SDL_GPU_TEXTUREFORMAT_INVALID;
    SDL_GPUTexture* virtualScreen_ = nullptr;
    SDL_GPUSampler* sampler_ = nullptr;
    SDL_GPUShader* spriteVertex_ = nullptr;
    SDL_GPUShader* spriteFragment_ = nullptr;
    SDL_GPUShader* blitVertex_ = nullptr;
    SDL_GPUShader* blitFragment_ = nullptr;
    SDL_GPUGraphicsPipeline* spriteNormal_ = nullptr;
    SDL_GPUGraphicsPipeline* spriteAdd_ = nullptr;
    SDL_GPUGraphicsPipeline* blitToWindow_ = nullptr;
    SDL_GPUGraphicsPipeline* blitToPicture_ = nullptr; // made when the first screenshot is taken
    SDL_GPUBuffer* vertexBuffer_ = nullptr;
    SDL_GPUTransferBuffer* vertexTransfer_ = nullptr;
    Uint32 vertexCapacity_ = 0;
    std::vector<GpuTexture> textures_;   // index = texture number
    std::vector<Vertex> vertices_;       // this frame's quads, six corners each, in drawing order
    std::vector<Batch> batches_;
    SDL_FColor clearColor_{0.0F, 0.0F, 0.0F, 1.0F};
};

} // namespace

bool gpuBackendCompiledIn() {
    return true;
}

std::unique_ptr<RenderBackend> makeGpuBackend(SDL_Window* window, int virtualWidth, int virtualHeight, odysseus::core::ScalingMode scaling) {
    return std::make_unique<GpuBackend>(window, virtualWidth, virtualHeight, scaling);
}

#else // no dxc.exe when this was built: the GPU backend is left out

bool gpuBackendCompiledIn() {
    return false;
}

std::unique_ptr<RenderBackend> makeGpuBackend(SDL_Window*, int, int, odysseus::core::ScalingMode) {
    throw std::runtime_error("this build has no GPU backend");
}

#endif

} // namespace luna::platform
