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

    void TEX::to_dds(std::ostringstream& outp_file) {
        BinaryWriter writer{ outp_file };

        if (this->signature != 0x00584554) {
            throw std::runtime_error("TEX Might not have been initialized (read) before converting to DDS");
        }

        DDSHeader header{};
        header.dwHeight = this->height;
        header.dwWidth = this->width;

        // Pixel format things uwu
        if (this->format == TEXFormat::DXT1) {
            header.ddspf.dwFlags = 0x00000004;
            header.ddspf.dwFourCC = 0x31545844;  // 'DXT1' as DWORD
        }
        else if (this->format == TEXFormat::DXT5) {
            header.ddspf.dwFlags = 0x00000004;
            header.ddspf.dwFourCC = 0x35545844;  // 'DXT5' as DWORD
        }
        else if (this->format == TEXFormat::BGRA8) {
            header.ddspf.dwFlags = 0x00000041;
            header.ddspf.dwRGBBitCount = 32;
            header.ddspf.dwBBitMask = 0x000000ff;
            header.ddspf.dwGBitMask = 0x0000ff00;
            header.ddspf.dwRBitMask = 0x00ff0000;
            header.ddspf.dwABitMask = 0xff000000;
        }
        else {
            throw std::runtime_error("Unsupported TEX format while setting up pixel format");
        }

        if (this->has_mipmaps) {
            header.dwFlags |= 0x00020000;
            header.dwCaps |= 0x00400008;
            header.dwMipMapCount = static_cast<std::uint32_t>(this->data.size());
        }

        writer.writeString("DDS ");
        {
            std::vector<char> header_data(sizeof(DDSHeader));
            std::memcpy(header_data.data(), &header, sizeof(DDSHeader));
            writer.write(header_data);
        }

        if (this->has_mipmaps) {
            // mipmap in tex file is reversed to dds file
            for (auto it = this->data.rbegin(); it != this->data.rend(); ++it) {
                writer.write_raw(it->data(), it->size());
            }
        } else {
            if (this->data.empty()) {
                throw std::runtime_error("TEX has no data to write to DDS");
            }
            
            writer.write_raw(this->data[0].data(), this->data[0].size());
        }
    }
}