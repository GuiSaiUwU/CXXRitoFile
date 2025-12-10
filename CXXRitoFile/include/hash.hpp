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

    inline std::uint32_t elf(const std::string_view& str) {
		std::uint32_t h = 0;
        
        for (std::uint32_t c : str) {
			h = (h << 4) + c;
			std::uint32_t t = (h & 0xF0000000);
            if (t != 0) {
                h ^= (t >> 24);
            }
			h &= ~t;
        }

        return h;
    }
}