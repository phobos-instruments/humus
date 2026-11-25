// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include <clap/stream.h>

namespace hum::claphost {

class WriteStream {
public:
    WriteStream() {
        stream_.ctx = this;
        stream_.write = [](const clap_ostream_t* s, const void* data, uint64_t size) -> int64_t {
            auto& self = *static_cast<WriteStream*>(s->ctx);
            const auto* bytes = static_cast<const uint8_t*>(data);
            self.bytes_.insert(self.bytes_.end(), bytes, bytes + size);
            return (int64_t) size;
        };
    }
    WriteStream(const WriteStream&) = delete;
    WriteStream& operator=(const WriteStream&) = delete;

    const clap_ostream_t* stream() const { return &stream_; }
    const std::vector<uint8_t>& bytes() const { return bytes_; }

private:
    std::vector<uint8_t> bytes_;
    clap_ostream_t stream_{};
};

class ReadStream {
public:
    ReadStream(const void* data, size_t size) : data_(static_cast<const uint8_t*>(data)), size_(size) {
        stream_.ctx = this;
        stream_.read = [](const clap_istream_t* s, void* out, uint64_t size) -> int64_t {
            auto& self = *static_cast<ReadStream*>(s->ctx);
            const size_t n = (size_t) std::min<uint64_t>(size, self.size_ - self.pos_);
            std::memcpy(out, self.data_ + self.pos_, n);
            self.pos_ += n;
            return (int64_t) n;
        };
    }
    ReadStream(const ReadStream&) = delete;
    ReadStream& operator=(const ReadStream&) = delete;

    const clap_istream_t* stream() const { return &stream_; }

private:
    const uint8_t* data_;
    size_t size_, pos_ = 0;
    clap_istream_t stream_{};
};

}
