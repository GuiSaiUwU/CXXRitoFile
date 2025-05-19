#pragma once
#include "structs.hpp"
#include "binary_stream.hpp"
#include <variant>


namespace RitoFile {
    enum BINType : unsigned char {
        // basic
        Empty = 0,
        Bool = 1,
        I8 = 2,
        U8 = 3,
        I16 = 4,
        U16 = 5,
        I32 = 6,
        U32 = 7,
        I64 = 8,
        U64 = 9,
        F32 = 10,
        Vec2 = 11,
        Vec3 = 12,
        Vec4 = 13,
        Mtx4 = 14,
        RGBA = 15,
        String = 16,
        Hash = 17,
        File = 18,
        // complex
        List = 128,
        List2 = 129,
        Pointer = 130,
        Embed = 131,
        Link = 132,
        Option = 133,
        Map = 134,
        Flag = 135
    };

    struct BINHelper {
        std::uint32_t bin_hash, hash_type;
        BINType type, value_type, key_type;
        std::variant <
            Container3<std::uint16_t>,
            bool,
            std::int8_t,
            std::uint8_t,
            std::int16_t,
            std::uint16_t,
            std::int32_t,
            std::uint32_t, // Hash too
            std::int64_t, // File too
            std::uint64_t,
            std::float_t,
            Container2<float>, // Vec2
            Container3<float>, // Vec3
            Container4<float>, // Vec4 and RGBA
            Matrix4,
            std::string,
            // complex
            List = 128,
            List2 = 129,
            Pointer = 130,
            Embed = 131,
            Link = 132,
            Option = 133,
            Map = 134,
            Flag = 135
        > data;
    };
}