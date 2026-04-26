#include <cstdint>
#include <vector>
#include <format>
#include "binary_stream.hpp"


/*
* Sources:
* - https://github.com/tarngaina/LtMAO/blob/hai/src/LtMAO/pyRitoFile/tex.py
* - https://github.com/Morilli/Ritoddstex/blob/2025af437c7882673d10e8de247541d3534e6f39/main.c
*/
namespace RitoFile {
    enum TEXFormat : unsigned char {
        ETC1 = 0x1,
        ETC2_EAC = 0x2,
        ETC2 = 0x3,
        DXT1 = 0xA,
        DXT5 = 0xC,
        BGRA8 = 0x14
    };

    class TEX {
    public:
        std::uint32_t signature;
        std::uint16_t width, height;
        std::uint8_t unknown1, format, unknown2;

        // vec of mipmaps, each inside = one layer
        std::vector<std::vector<char>> data;
        bool has_mipmaps;

        BinaryReader reader;

		TEX(std::stringstream& inpt_file);
		void read();
		void write(std::ostringstream& outp_file);

    private:
        inline std::uint32_t mipmap_count() {
            std::uint32_t num = 0;

            auto cur_width = width;
            auto cur_height = height;

            while (cur_width > 1 || cur_height > 1) {
                if (cur_width > 1) cur_width >>= 1;
                if (cur_height > 1) cur_height >>= 1;
                ++num;
            }
            return num;
        }
    };
}