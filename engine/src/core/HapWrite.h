#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/DxtEncode.h"
#include "core/MovWrite.h"
#include "core/SnappyEncode.h"

namespace hum::hap {

namespace write {

inline mov::Bytes frameSection(const mov::Bytes& payload, std::uint8_t type) {
    mov::Bytes head;
    const auto n = payload.size();
    if (n < 0xFFFFFF) {
        head.push_back((std::uint8_t) (n & 0xFF));
        head.push_back((std::uint8_t) ((n >> 8) & 0xFF));
        head.push_back((std::uint8_t) ((n >> 16) & 0xFF));
        head.push_back(type);
    } else {
        head.insert(head.end(), {0, 0, 0, type});
        for (int i = 0; i < 4; ++i) head.push_back((std::uint8_t) ((n >> (8 * i)) & 0xFF));
    }
    return head;
}

}

class Writer {
public:
    Writer(const juce::File& file, int width, int height, double fps) {
        picture_.width = width;
        picture_.height = height;
        if (width <= 0 || height <= 0 || fps <= 0.0) return;
        picture_.timescale = (std::uint32_t) std::max(1LL, std::llround(fps * 1000.0));
        picture_.delta = 1000;
        file.deleteFile();
        out_ = file.createOutputStream();
        if (out_ == nullptr) return;
        mov::Bytes head;
        mov::tag(head, "qt  ");
        mov::u32(head, 0x00000200);
        mov::tag(head, "qt  ");
        const auto ftyp = mov::box("ftyp", head);
        out_->write(ftyp.data(), ftyp.size());
        mdatAt_ = out_->getPosition();
        mov::Bytes mdat;
        mov::u32(mdat, 1);
        mov::tag(mdat, "mdat");
        mov::zeros(mdat, 8);
        out_->write(mdat.data(), mdat.size());
        picture_.offset = out_->getPosition();
        open_ = true;
    }

    ~Writer() { close(); }

    bool ok() const { return open_; }
    int frameCount() const { return (int) picture_.sizes.size(); }
    int soundFrameCount() const { return (int) sound_.frames; }

    bool openSound(int channels, double sampleRate) {
        if (!open_ || !picture_.sizes.empty() || channels <= 0 || sampleRate <= 0.0) return false;
        sound_.channels = channels;
        sound_.sampleRate = sampleRate;
        sound_.offset = out_->getPosition();
        return true;
    }

    bool addSound(const float* const* channels, int count, int frames) {
        if (!open_ || sound_.channels <= 0 || count != sound_.channels || frames <= 0) return false;
        if (!picture_.sizes.empty()) return false;
        mov::interleave24(channels, count, frames, packed_);
        if (!out_->write(packed_.data(), packed_.size())) return false;
        sound_.frames += (std::uint32_t) frames;
        return true;
    }

    bool addFrame(const std::uint8_t* rgba) {
        if (!open_ || rgba == nullptr) return false;
        if (picture_.sizes.empty()) picture_.offset = out_->getPosition();
        dxt::encodeDxt1(rgba, picture_.width, picture_.height, blocks_);
        const bool squeezed = snappy::encode(blocks_.data(), blocks_.size(), packed_)
                              && packed_.size() < blocks_.size();
        const auto& body = squeezed ? packed_ : blocks_;
        const auto head = write::frameSection(body, squeezed ? 0xBB : 0xAB);
        if (!out_->write(head.data(), head.size()) || !out_->write(body.data(), body.size()))
            return false;
        picture_.sizes.push_back((std::uint32_t) (head.size() + body.size()));
        return true;
    }

    bool close() {
        if (!open_) return false;
        open_ = false;
        const auto end = out_->getPosition();
        const auto moov = mov::movieHeader(picture_, sound_);
        bool wrote = out_->write(moov.data(), moov.size());
        const auto span = (std::uint64_t) (end - mdatAt_);
        mov::Bytes size;
        for (int shift = 56; shift >= 0; shift -= 8)
            size.push_back((std::uint8_t) ((span >> shift) & 0xFF));
        wrote = out_->setPosition(mdatAt_ + 8) && out_->write(size.data(), size.size()) && wrote;
        out_->flush();
        out_.reset();
        return wrote && !picture_.sizes.empty();
    }

private:
    mov::Picture picture_;
    mov::Sound sound_;
    std::int64_t mdatAt_ = 0;
    bool open_ = false;
    std::unique_ptr<juce::FileOutputStream> out_;
    std::vector<std::uint8_t> blocks_, packed_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Writer)
};

}
