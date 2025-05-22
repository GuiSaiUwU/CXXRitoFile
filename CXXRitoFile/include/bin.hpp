#pragma once
#include "structs.hpp"
#include "binary_stream.hpp"
#include <variant>


namespace RitoFile {
    struct BINField;

    using BinBasicVariant = std::variant<
        Container3<std::uint16_t>,
        bool,
        std::int8_t,
        std::uint8_t,
        std::int16_t,
        std::uint16_t,
        std::int32_t,
        std::uint32_t, // Hash & Link
        std::int64_t, // File
        std::uint64_t,
        std::float_t,
        Container2<float>, // Vec2
        Container3<float>, // Vec3
        Container4<float>, // Vec4 and RGBA
        Matrix4,
        std::string
    >;
    
    using BinDataVariant = std::variant <
        BinBasicVariant,
        std::vector<BinBasicVariant>, // List & List2
        std::vector<BINField>, // Pointer & Embed
        std::vector<std::tuple<BinBasicVariant, BinBasicVariant>> // Map
    >;

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

    struct BINField {
        std::uint32_t bin_hash, hash_type;
        BINType type, value_type, key_type;
        BinDataVariant data;
    };

    struct BINEntry {
        std::uint32_t bin_hash;
        BINType type;
        BinDataVariant data;
    };

    class BIN {
    public:
        std::vector<std::string> links;
        std::vector<BINEntry> entries;
        BinaryReader reader;

        BIN(std::ifstream& inpt_file);
        void read();
        void write(std::ostringstream& outp_file);
    };
}