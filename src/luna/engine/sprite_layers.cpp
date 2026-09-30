#include "luna/engine/sprite_layers.h"

namespace luna::engine {

Image recoloured(const Image& layer, const std::vector<ColourSwap>& swaps) {
    Image out = layer;
    for (int y = 0; y < out.height(); ++y) {
        for (int x = 0; x < out.width(); ++x) {
            const Color c = out.get(x, y);
            for (const ColourSwap& swap : swaps) {
                if (c.red == swap.from.red && c.green == swap.from.green && c.blue == swap.from.blue) {
                    out.set(x, y, Color{swap.to.red, swap.to.green, swap.to.blue, c.alpha});
                    break;
                }
            }
        }
    }
    return out;
}

Image composed(const std::vector<const Image*>& layersBottomFirst) {
    if (layersBottomFirst.empty() || layersBottomFirst.front() == nullptr) {
        return Image(1, 1);
    }
    Image out(layersBottomFirst.front()->width(), layersBottomFirst.front()->height());
    for (const Image* layer : layersBottomFirst) {
        if (layer == nullptr || layer->width() != out.width() || layer->height() != out.height()) {
            continue;
        }
        for (int y = 0; y < out.height(); ++y) {
            for (int x = 0; x < out.width(); ++x) {
                const Color top = layer->get(x, y);
                if (top.alpha == 0) {
                    continue;
                }
                if (top.alpha == 255) {
                    out.set(x, y, top);
                    continue;
                }
                const Color below = out.get(x, y);
                const int a = top.alpha;
                auto mix = [&](int t, int b) { return static_cast<std::uint8_t>((t * a + b * (255 - a)) / 255); };
                out.set(x, y, Color{mix(top.red, below.red), mix(top.green, below.green), mix(top.blue, below.blue),
                                    static_cast<std::uint8_t>(a + below.alpha * (255 - a) / 255)});
            }
        }
    }
    return out;
}

} // namespace luna::engine
