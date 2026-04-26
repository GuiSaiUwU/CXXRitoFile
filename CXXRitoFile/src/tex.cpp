#include "tex.hpp"

namespace RitoFile {
    TEX::TEX(std::stringstream &inpt_file) : reader(inpt_file)
    {
        if (!inpt_file.good()) {
			throw std::runtime_error("Stream not good");
		}
    }

    void TEX::read() {
        this->signature = reader.readU32();
        if (this->signature != 0x00584554) {
            throw std::runtime_error("Invalid TEX signature: " + std::to_string(this->signature));
        }

        this->width = reader.readU16();
        this->height = reader.readU16();
        
        this->unknown1 = reader.readU8();
        this->format = TEXFormat(reader.readU8());
        this->unknown2 = reader.readU8();

        this->has_mipmaps = reader.readBool();
        
        if (this->has_mipmaps
        && (this->format == DXT1 || this->format == DXT5 || this->format == BGRA8)) {
            std::uint8_t block_size, bytes_per_block;
            if (this->format == DXT1) {
                block_size = 4;
                bytes_per_block = 8;
            } else if (this->format == DXT5) {
                block_size = 4;
                bytes_per_block = 16;
            } else {
                block_size = 1;
                bytes_per_block = 4;
            }

            std::uint32_t mipmap_count = this->mipmap_count();
            this->data.resize(mipmap_count + 1 /* main texture */);

            for (std::int32_t x = mipmap_count; x >= 0; x--) {
                int curr_width = std::max(width / (1 << x), 1);
                int curr_height = std::max(height / (1 << x), 1);

                int block_width = (curr_width + block_size - 1) / block_size;
                int block_height = (curr_height + block_size - 1) / block_size;

                std::uint32_t current_size = bytes_per_block  * block_width * block_height;
                std::cout << std::format("Mipmap {}: {}x{}, block: {}x{}, size: {} bytes\n", mipmap_count - x, curr_width, curr_height, block_width, block_height, current_size);

                reader.readBuffered(this->data.at(mipmap_count - x), current_size);
            }
        } else {
            this->data.resize(1);
            this->data.emplace_back(reader.readRemaining());
        }
    }

    void TEX::write(std::ostringstream& outp_file) {
        BinaryWriter writer{ outp_file };

        writer.writeU32(0x00584554);
        writer.writeU16(this->width);
        writer.writeU16(this->height);
        writer.writeU8(1); // unknown1
        writer.writeU8(this->format);
        writer.writeU8(0); // unknown2
        writer.writeBool(this->has_mipmaps);
        if (this->has_mipmaps
        && (this->format == DXT1 || this->format == DXT5 || this->format == BGRA8)) {
            for (auto& mipmap : this->data) {
                writer.write(mipmap);
            }
        } else {
            writer.write(this->data.at(0));
        }
    }
}