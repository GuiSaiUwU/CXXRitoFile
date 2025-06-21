#pragma once
#include "structs.hpp"
#include "binary_stream.hpp"
#include <format>
#include <array>
#include <algorithm> // std::find
#include <unordered_map>
#include <any>



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

    struct BINField {
        std::uint32_t bin_hash, hash_type;
        BINType type, value_type, key_type;
        std::any data;
    };

    struct BINEntry {
        std::uint32_t bin_hash;
        std::uint32_t type;
        std::vector<BINField> data;
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

    struct BINHelper {

        static std::any readValue(BinaryReader& reader, BINType value_type);
        static BINField readField(BinaryReader& reader);
    };
}