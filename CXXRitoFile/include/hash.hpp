#pragma once
#include <string>
#include <cstdint>

/* Third Party */
#include "xxhash64.h"
#define xxh64(inpt, size) XXHash64::hash(inpt, size, 0)

namespace RitoFile {
    std::uint32_t fnv1a_lower_cased(const std::string_view& str) noexcept;
}