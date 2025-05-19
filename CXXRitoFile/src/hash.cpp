#include "hash.hpp"

namespace RitoFile {
    std::uint32_t inline fnv1a_lower_cased(const std::string_view& str) noexcept {
        std::uint32_t h = 0x811c9dc5;
        for (std::uint32_t c : str) {
            c = c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c; // lowercase
            h = ((h ^ c) * 0x01000193) & 0xFFFFFFFF;
        }

        return h;
    }
}
