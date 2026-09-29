#pragma once

#include "boundary.h"

#include <cstdint>
#include <string_view>

namespace odysseus::core {

// FNV-1a, 64 bit: turns any amount of data into one number. Two worlds with equal state
// give equal hashes; one different bit almost certainly changes the hash. Used by the
// determinism test (ADR-011) and to check that saves load exactly (US-016).
class Hasher {
public:
    void add(std::uint64_t value) {
        for (int byte = 0; byte < 8; ++byte) {
            addByte(static_cast<std::uint8_t>(value >> (byte * 8)));
        }
    }
    void add(std::int64_t value) { add(static_cast<std::uint64_t>(value)); }
    void add(int value) { add(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))); }
    void add(bool value) { addByte(value ? 1U : 0U); }
    void add(std::string_view text) {
        add(static_cast<std::uint64_t>(text.size()));
        for (const char c : text) {
            addByte(static_cast<std::uint8_t>(c));
        }
    }

    std::uint64_t value() const { return hash_; }

private:
    void addByte(std::uint8_t byte) {
        hash_ ^= byte;
        hash_ *= 1099511628211ULL; // FNV prime
    }

    std::uint64_t hash_ = 14695981039346656037ULL; // FNV offset basis
};

} // namespace odysseus::core
