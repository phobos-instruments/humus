#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

class HandNet {
public:
    enum Kind { Conv = 1, Depthwise, Add, PRelu, MaxPool, FullyConnected,
                Logistic, Mean, Pad, Concat, Reshape, ResizeBilinear };

    struct Tensor {
        int shape[4] = {1, 1, 1, 1};
        int rank = 0;
        std::vector<float> data;
        bool constant = false;
        int count() const { return shape[0] * shape[1] * shape[2] * shape[3]; }
    };

    struct Op {
        int kind = 0;
        int p[5] = {};
        std::vector<int> in, out;
    };

    bool ok() const { return ok_; }
    const std::string& error() const { return err_; }

    static bool loadFile(const std::string& path, HandNet& detector, HandNet& landmarks);

    bool run(const float* input, int inputCount);
    const Tensor& output(int i) const { return tensors_[(size_t) outputs_[(size_t) i]]; }
    int outputCount() const { return (int) outputs_.size(); }
    const Tensor& tensor(int i) const { return tensors_[(size_t) i]; }

    bool parse(const uint8_t* p, size_t n);

    bool trace = false;

    void dumpJoins() const {
        for (size_t i = 0; i < ops_.size(); ++i) {
            const auto& o = ops_[i];
            if (o.kind != Concat && o.kind != Reshape) continue;
            std::printf("  op %3zu %s out[%d] %dx%dx%d rank %d <-", i,
                        o.kind == Concat ? "CONCAT " : "RESHAPE",
                        o.out[0], tensors_[(size_t) o.out[0]].shape[1],
                        tensors_[(size_t) o.out[0]].shape[2],
                        tensors_[(size_t) o.out[0]].shape[3],
                        tensors_[(size_t) o.out[0]].rank);
            for (int t : o.in)
                std::printf("  [%d] %dx%dx%d", t, tensors_[(size_t) t].shape[1],
                            tensors_[(size_t) t].shape[2], tensors_[(size_t) t].shape[3]);
            if (o.kind == Concat) std::printf("   axis %d", o.p[0]);
            std::printf("\n");
        }
        std::printf("  outputs:");
        for (int t : outputs_) std::printf(" [%d]", t);
        std::printf("\n");
    }

private:
    void exec(const Op& op);
    void conv(const Op& op, bool depthwise);
    void pool(const Op& op);
    void dense(const Op& op);

    std::vector<Tensor> tensors_;
    std::vector<Op> ops_;
    std::vector<int> inputs_, outputs_;
    bool ok_ = false;
    std::string err_;
};

namespace handnet_detail {

inline float half2float(uint16_t h) {
    const uint32_t sign = (uint32_t) (h & 0x8000u) << 16;
    const uint32_t exp = (h >> 10) & 0x1Fu;
    const uint32_t man = h & 0x3FFu;
    uint32_t bits;
    if (exp == 0) {
        if (man == 0) bits = sign;
        else {
            int e = -1;
            uint32_t m = man;
            do { ++e; m <<= 1; } while ((m & 0x400u) == 0);
            bits = sign | ((uint32_t) (127 - 15 - e) << 23) | ((m & 0x3FFu) << 13);
        }
    } else if (exp == 31) {
        bits = sign | 0x7F800000u | (man << 13);
    } else {
        bits = sign | ((exp + 127 - 15) << 23) | (man << 13);
    }
    float f;
    std::memcpy(&f, &bits, 4);
    return f;
}

inline float act(float v, int kind) {
    if (kind == 1) return v > 0.0f ? v : 0.0f;
    if (kind == 3) return v < 0.0f ? 0.0f : (v > 6.0f ? 6.0f : v);
    return v;
}

inline int padFor(int inSize, int outSize, int k, int stride, bool valid) {
    if (valid) return 0;
    const int need = (outSize - 1) * stride + k - inSize;
    return need > 0 ? need / 2 : 0;
}

struct Reader {
    const uint8_t* p; size_t n, at = 0;
    bool need(size_t k) const { return at + k <= n; }
    uint32_t u32() { uint32_t v; std::memcpy(&v, p + at, 4); at += 4; return v; }
    int32_t i32() { int32_t v; std::memcpy(&v, p + at, 4); at += 4; return v; }
    uint8_t u8() { return p[at++]; }
};

}

inline bool HandNet::parse(const uint8_t* p, size_t n) {
    using namespace handnet_detail;
    Reader r{p, n};
    if (!r.need(24) || std::memcmp(p, "HNET", 4) != 0) { err_ = "not a HNET part"; return false; }
    r.at = 4;
    const uint32_t version = r.u32();
    if (version != 1) { err_ = "HNET version " + std::to_string(version); return false; }
    const uint32_t nT = r.u32(), nO = r.u32(), nIn = r.u32(), nOut = r.u32();

    inputs_.resize(nIn);
    for (auto& v : inputs_) v = r.i32();
    outputs_.resize(nOut);
    for (auto& v : outputs_) v = r.i32();

    const uint32_t tBytes = r.u32(), oBytes = r.u32(), bBytes = r.u32();
    const size_t tAt = r.at, oAt = tAt + tBytes, bAt = oAt + oBytes;
    if (bAt + bBytes > n) { err_ = "truncated"; return false; }
    const uint8_t* blob = p + bAt;

    tensors_.resize(nT);
    for (uint32_t i = 0; i < nT; ++i) {
        Reader t{p, n, tAt + (size_t) i * 26};
        auto& d = tensors_[i];
        const int type = t.u8();
        d.rank = t.u8();
        for (int k = 0; k < 4; ++k) d.shape[k] = t.i32();
        if (d.rank > 0 && d.rank < 4) {
            int tmp[4] = {1, 1, 1, 1};
            for (int k = 0; k < d.rank; ++k) tmp[4 - d.rank + k] = d.shape[k];
            for (int k = 0; k < 4; ++k) d.shape[k] = tmp[k];
        }
        const uint32_t off = t.u32(), bytes = t.u32();
        if (bytes == 0) { d.data.assign((size_t) d.count(), 0.0f); continue; }
        d.constant = true;
        const size_t count = (size_t) d.count();
        d.data.resize(count);
        if (type == 1) {
            for (size_t k = 0; k < count; ++k) {
                uint16_t h;
                std::memcpy(&h, blob + off + k * 2, 2);
                d.data[k] = half2float(h);
            }
        } else if (type == 2) {
            for (size_t k = 0; k < count; ++k) {
                int32_t v;
                std::memcpy(&v, blob + off + k * 4, 4);
                d.data[k] = (float) v;
            }
        } else {
            std::memcpy(d.data.data(), blob + off, count * 4);
        }
    }

    ops_.resize(nO);
    size_t at = oAt;
    for (uint32_t i = 0; i < nO; ++i) {
        Reader o{p, n, at};
        auto& op = ops_[i];
        op.kind = o.u8();
        for (int k = 0; k < 5; ++k) op.p[k] = o.i32();
        const int ni = o.u8(), no = o.u8();
        op.in.resize((size_t) ni);
        for (auto& v : op.in) v = o.i32();
        op.out.resize((size_t) no);
        for (auto& v : op.out) v = o.i32();
        at = o.at;
    }
    ok_ = true;
    return true;
}

inline void HandNet::conv(const Op& op, bool depthwise) {
    using namespace handnet_detail;
    const auto& in = tensors_[(size_t) op.in[0]];
    const auto& w = tensors_[(size_t) op.in[1]];
    const auto& b = tensors_[(size_t) op.in[2]];
    auto& out = tensors_[(size_t) op.out[0]];

    const int ih = in.shape[1], iw = in.shape[2], ic = in.shape[3];
    const int oh = out.shape[1], ow = out.shape[2], oc = out.shape[3];
    const int kh = w.shape[1], kw = w.shape[2];
    const bool valid = op.p[0] == 1;
    const int sw = op.p[1], sh = op.p[2], fused = op.p[3];
    const int padY = padFor(ih, oh, kh, sh, valid);
    const int padX = padFor(iw, ow, kw, sw, valid);

    if (!depthwise && kh == 1 && kw == 1 && sh == 1 && sw == 1 && padY == 0 && padX == 0) {
        const int pixels = oh * ow;
        for (int i = 0; i < pixels; ++i) {
            const float* ip = in.data.data() + (size_t) i * ic;
            float* op = out.data.data() + (size_t) i * oc;
            for (int o = 0; o < oc; ++o) {
                const float* kp = w.data.data() + (size_t) o * ic;
                float a0 = 0.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
                int c = 0;
                for (; c + 4 <= ic; c += 4) {
                    a0 += ip[c] * kp[c];
                    a1 += ip[c + 1] * kp[c + 1];
                    a2 += ip[c + 2] * kp[c + 2];
                    a3 += ip[c + 3] * kp[c + 3];
                }
                float acc = b.data[(size_t) o] + a0 + a1 + a2 + a3;
                for (; c < ic; ++c) acc += ip[c] * kp[c];
                op[o] = act(acc, fused);
            }
        }
        return;
    }

    for (int y = 0; y < oh; ++y)
        for (int x = 0; x < ow; ++x) {
            const int iy0 = y * sh - padY, ix0 = x * sw - padX;
            const int ky0 = std::max(0, -iy0), ky1 = std::min(kh, ih - iy0);
            const int kx0 = std::max(0, -ix0), kx1 = std::min(kw, iw - ix0);
            if (depthwise) {
                float* op = out.data.data() + (size_t) (y * ow + x) * oc;
                for (int c = 0; c < oc; ++c) op[c] = b.data[(size_t) c];
                for (int ky = ky0; ky < ky1; ++ky)
                    for (int kx = kx0; kx < kx1; ++kx) {
                        const float* ip = in.data.data()
                                        + (size_t) ((iy0 + ky) * iw + ix0 + kx) * ic;
                        const float* kp = w.data.data() + (size_t) (ky * kw + kx) * oc;
                        for (int c = 0; c < oc; ++c) op[c] += ip[c] * kp[c];
                    }
                for (int c = 0; c < oc; ++c) op[c] = act(op[c], fused);
                continue;
            }
            for (int o = 0; o < oc; ++o) {
                float acc = b.data[(size_t) o];
                const float* wp = w.data.data() + (size_t) o * kh * kw * ic;
                for (int ky = ky0; ky < ky1; ++ky)
                    for (int kx = kx0; kx < kx1; ++kx) {
                        const float* ip = in.data.data()
                                        + (size_t) ((iy0 + ky) * iw + ix0 + kx) * ic;
                        const float* kp = wp + (size_t) (ky * kw + kx) * ic;
                        for (int c = 0; c < ic; ++c) acc += ip[c] * kp[c];
                    }
                out.data[(size_t) ((y * ow + x) * oc + o)] = act(acc, fused);
            }
        }
}

inline void HandNet::pool(const Op& op) {
    const auto& in = tensors_[(size_t) op.in[0]];
    auto& out = tensors_[(size_t) op.out[0]];
    const int ih = in.shape[1], iw = in.shape[2], c = in.shape[3];
    const int oh = out.shape[1], ow = out.shape[2];
    const bool valid = op.p[0] == 1;
    const int sw = op.p[1], sh = op.p[2];
    const int fw = (op.p[4] >> 8) & 0xFF, fh = op.p[4] & 0xFF;
    const int padY = handnet_detail::padFor(ih, oh, fh, sh, valid);
    const int padX = handnet_detail::padFor(iw, ow, fw, sw, valid);
    for (int y = 0; y < oh; ++y)
        for (int x = 0; x < ow; ++x)
            for (int ch = 0; ch < c; ++ch) {
                float best = -3.4e38f;
                for (int ky = 0; ky < fh; ++ky) {
                    const int iy = y * sh - padY + ky;
                    if (iy < 0 || iy >= ih) continue;
                    for (int kx = 0; kx < fw; ++kx) {
                        const int ix = x * sw - padX + kx;
                        if (ix < 0 || ix >= iw) continue;
                        best = std::max(best, in.data[(size_t) ((iy * iw + ix) * c + ch)]);
                    }
                }
                out.data[(size_t) ((y * ow + x) * c + ch)] = best;
            }
}

inline void HandNet::dense(const Op& op) {
    const auto& in = tensors_[(size_t) op.in[0]];
    const auto& w = tensors_[(size_t) op.in[1]];
    const auto& b = tensors_[(size_t) op.in[2]];
    auto& out = tensors_[(size_t) op.out[0]];
    const int units = w.shape[2], depth = w.shape[3];
    for (int u = 0; u < units; ++u) {
        float acc = b.count() > u ? b.data[(size_t) u] : 0.0f;
        const float* wp = w.data.data() + (size_t) u * depth;
        for (int k = 0; k < depth; ++k) acc += in.data[(size_t) k] * wp[k];
        out.data[(size_t) u] = handnet_detail::act(acc, op.p[3]);
    }
}

inline void HandNet::exec(const Op& op) {
    using namespace handnet_detail;
    switch (op.kind) {
        case Conv:      conv(op, false); break;
        case Depthwise: conv(op, true); break;
        case MaxPool:   pool(op); break;
        case FullyConnected: dense(op); break;
        case Add: {
            const auto& a = tensors_[(size_t) op.in[0]];
            const auto& b = tensors_[(size_t) op.in[1]];
            auto& o = tensors_[(size_t) op.out[0]];
            const int n = o.count(), na = a.count(), nb = b.count();
            for (int i = 0; i < n; ++i)
                o.data[(size_t) i] = a.data[(size_t) (na == n ? i : i % na)]
                                   + b.data[(size_t) (nb == n ? i : i % nb)];
            break;
        }
        case PRelu: {
            const auto& a = tensors_[(size_t) op.in[0]];
            const auto& s = tensors_[(size_t) op.in[1]];
            auto& o = tensors_[(size_t) op.out[0]];
            const int n = o.count(), ns = s.count();
            for (int i = 0; i < n; ++i) {
                const float v = a.data[(size_t) i];
                o.data[(size_t) i] = v >= 0.0f ? v
                                    : v * s.data[(size_t) (ns == n ? i : i % ns)];
            }
            break;
        }
        case Logistic: {
            const auto& a = tensors_[(size_t) op.in[0]];
            auto& o = tensors_[(size_t) op.out[0]];
            for (int i = 0; i < o.count(); ++i)
                o.data[(size_t) i] = 1.0f / (1.0f + std::exp(-a.data[(size_t) i]));
            break;
        }
        case Mean: {
            const auto& a = tensors_[(size_t) op.in[0]];
            auto& o = tensors_[(size_t) op.out[0]];
            const int h = a.shape[1], w = a.shape[2], c = a.shape[3];
            for (int ch = 0; ch < c; ++ch) {
                float acc = 0.0f;
                for (int i = 0; i < h * w; ++i) acc += a.data[(size_t) (i * c + ch)];
                o.data[(size_t) ch] = acc / (float) (h * w);
            }
            break;
        }
        case Pad: {
            const auto& a = tensors_[(size_t) op.in[0]];
            const auto& pd = tensors_[(size_t) op.in[1]];
            auto& o = tensors_[(size_t) op.out[0]];
            std::fill(o.data.begin(), o.data.end(), 0.0f);
            const int top = (int) pd.data[2], left = (int) pd.data[4];
            const int front = pd.count() >= 8 ? (int) pd.data[6] : 0;
            const int ih = a.shape[1], iw = a.shape[2], ic = a.shape[3];
            const int ow = o.shape[2], oc = o.shape[3];
            for (int y = 0; y < ih; ++y)
                for (int x = 0; x < iw; ++x)
                    std::memcpy(o.data.data()
                                    + (size_t) (((y + top) * ow + (x + left)) * oc + front),
                                a.data.data() + (size_t) (y * iw + x) * ic,
                                (size_t) ic * sizeof(float));
            break;
        }
        case Concat: {
            auto& o = tensors_[(size_t) op.out[0]];
            const int pad = 4 - (o.rank > 0 ? o.rank : 4);
            const int axis = std::clamp(op.p[0] + pad, 0, 3);
            int outer = 1, inner = 1;
            for (int d = 0; d < axis; ++d) outer *= o.shape[d];
            for (int d = axis + 1; d < 4; ++d) inner *= o.shape[d];
            int base = 0;
            for (int t : op.in) {
                const auto& a = tensors_[(size_t) t];
                const int span = a.shape[axis];
                for (int b = 0; b < outer; ++b)
                    std::memcpy(o.data.data()
                                    + ((size_t) b * o.shape[axis] + base) * inner,
                                a.data.data() + (size_t) b * span * inner,
                                (size_t) span * inner * sizeof(float));
                base += span;
            }
            break;
        }
        case Reshape: {
            const auto& a = tensors_[(size_t) op.in[0]];
            auto& o = tensors_[(size_t) op.out[0]];
            std::memcpy(o.data.data(), a.data.data(),
                        (size_t) std::min(a.count(), o.count()) * sizeof(float));
            break;
        }
        case ResizeBilinear: {
            const auto& a = tensors_[(size_t) op.in[0]];
            auto& o = tensors_[(size_t) op.out[0]];
            const int ih = a.shape[1], iw = a.shape[2], c = a.shape[3];
            const int oh = o.shape[1], ow = o.shape[2];
            const bool alignCorners = op.p[0] != 0, halfPixel = op.p[4] != 0;
            const float sy = alignCorners && oh > 1 ? (float) (ih - 1) / (float) (oh - 1)
                                                    : (float) ih / (float) oh;
            const float sx = alignCorners && ow > 1 ? (float) (iw - 1) / (float) (ow - 1)
                                                    : (float) iw / (float) ow;
            for (int y = 0; y < oh; ++y) {
                float fy = halfPixel ? ((float) y + 0.5f) * sy - 0.5f : (float) y * sy;
                fy = std::max(0.0f, fy);
                const int y0 = (int) fy, y1 = std::min(y0 + 1, ih - 1);
                const float wy = fy - (float) y0;
                for (int x = 0; x < ow; ++x) {
                    float fx = halfPixel ? ((float) x + 0.5f) * sx - 0.5f : (float) x * sx;
                    fx = std::max(0.0f, fx);
                    const int x0 = (int) fx, x1 = std::min(x0 + 1, iw - 1);
                    const float wx = fx - (float) x0;
                    for (int ch = 0; ch < c; ++ch) {
                        const float p00 = a.data[(size_t) ((y0 * iw + x0) * c + ch)];
                        const float p01 = a.data[(size_t) ((y0 * iw + x1) * c + ch)];
                        const float p10 = a.data[(size_t) ((y1 * iw + x0) * c + ch)];
                        const float p11 = a.data[(size_t) ((y1 * iw + x1) * c + ch)];
                        o.data[(size_t) ((y * ow + x) * c + ch)] =
                            (p00 * (1 - wx) + p01 * wx) * (1 - wy)
                          + (p10 * (1 - wx) + p11 * wx) * wy;
                    }
                }
            }
            break;
        }
        default: break;
    }
}

inline bool HandNet::run(const float* input, int inputCount) {
    if (!ok_ || inputs_.empty()) return false;
    auto& in = tensors_[(size_t) inputs_[0]];
    if (in.count() != inputCount) return false;
    std::memcpy(in.data.data(), input, (size_t) inputCount * sizeof(float));
    for (size_t i = 0; i < ops_.size(); ++i) {
        exec(ops_[i]);
        if (!trace) continue;
        const auto& o = tensors_[(size_t) ops_[i].out[0]];
        const auto& a = tensors_[(size_t) ops_[i].in[0]];
        float lo = 1e30f, hi = -1e30f;
        for (int k = 0; k < o.count(); ++k) {
            lo = std::min(lo, o.data[(size_t) k]);
            hi = std::max(hi, o.data[(size_t) k]);
        }
        float th = -1e30f, bh = -1e30f;
        const int H = o.shape[1], W = o.shape[2], C = o.shape[3];
        for (int y = 0; y < H; ++y)
            for (int k = 0; k < W * C; ++k) {
                const float v = std::abs(o.data[(size_t) (y * W * C + k)]);
                (y < H / 2 ? th : bh) = std::max(y < H / 2 ? th : bh, v);
            }
        int ay = 0, ax = 0;
        float amax = -1e30f;
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                for (int c = 0; c < C; ++c) {
                    const float v = std::abs(o.data[(size_t) ((y * W + x) * C + c)]);
                    if (v > amax) { amax = v; ay = y; ax = x; }
                }
        std::printf("  %3zu kind %2d  in %dx%dx%d -> out %dx%dx%d   [%.3f .. %.3f]"
                    "  top %.3f bottom %.3f  peak (%.2f,%.2f)\n",
                    i, ops_[i].kind, a.shape[1], a.shape[2], a.shape[3],
                    o.shape[1], o.shape[2], o.shape[3], (double) lo, (double) hi,
                    (double) th, (double) bh,
                    (double) ay / (double) std::max(1, H), (double) ax / (double) std::max(1, W));
    }
    return true;
}

inline bool HandNet::loadFile(const std::string& path, HandNet& detector,
                              HandNet& landmarks) {
    juce::MemoryBlock mb;
    if (!juce::File(juce::String(path)).loadFileAsData(mb)) return false;
    std::vector<uint8_t> raw((const uint8_t*) mb.getData(),
                             (const uint8_t*) mb.getData() + mb.getSize());
    if (raw.size() > 2 && raw[0] == 0x1F && raw[1] == 0x8B) {
        juce::MemoryInputStream in(mb, false);
        juce::GZIPDecompressorInputStream gz(&in, false,
                                             juce::GZIPDecompressorInputStream::gzipFormat);
        juce::MemoryOutputStream out;
        out.writeFromInputStream(gz, -1);
        raw.assign((const uint8_t*) out.getData(),
                   (const uint8_t*) out.getData() + out.getDataSize());
    }
    if (raw.size() < 12 || std::memcmp(raw.data(), "HAND", 4) != 0) return false;
    uint32_t parts;
    std::memcpy(&parts, raw.data() + 8, 4);
    if (parts != 2) return false;
    uint32_t len[2];
    std::memcpy(len, raw.data() + 12, 8);
    const uint8_t* p = raw.data() + 20;
    return detector.parse(p, len[0]) && landmarks.parse(p + len[0], len[1]);
}

}
