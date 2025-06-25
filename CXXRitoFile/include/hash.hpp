#pragma once
#include <string>
#include <cstdint>

/* Third Party */
#include "xxhash64.h"
#define xxh64(inpt, size) XXHash64::hash(inpt, size, 0)

namespace RitoFile {
    inline std::uint32_t constexpr fnv1a(const std::string_view& str) noexcept {
        std::uint32_t h = 0x811c9dc5;
        for (std::uint32_t c : str) {
            c = c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c; // lowercase
            h = ((h ^ c) * 0x01000193) & 0xFFFFFFFF;
        }

        return h;
    }
}