#include "bin.hpp"


namespace RitoFile {
	BIN::BIN(std::ifstream& inpt_file) : reader(inpt_file) {
		if (!inpt_file.good()) {
			throw std::runtime_error("Stream not good");
		}
	}

	void BIN::read() {
		auto signature = reader.readString(4);
		if (signature != "PROP") {
			throw std::runtime_error(std::format("Only supports PROP signature, current: {}", signature));
		}

		auto version = reader.readU32();
		static std::array<int, 3> allowed_versions{(1, 2, 3)};
		if (std::find(allowed_versions.begin(), allowed_versions.end(), version) == allowed_versions.end()) {
			throw std::runtime_error(std::format("Version not supported: {}", version));
		}

		if (version >= 2) {
			auto link_count = reader.readU32();
			this->links.reserve(link_count);
			for (auto _i = 0; _i < link_count; _i++) {
				this->links.emplace_back(reader.readStringSized16());
			}
		}

		auto entry_count = reader.readU32();
		std::vector<std::uint32_t> entry_types(entry_count);
		reader.readBuffered<std::uint32_t>(entry_types, entry_count * sizeof(std::uint32_t));

		/*
		self.entries = [BINEntry() for i in range(entry_count)]
        for entry_id, entry in enumerate(self.entries):
            entry.type = hash_to_hex(entry_types[entry_id])
            bs.pad(4)  # size
            entry.hash = hash_to_hex(bs.read_u32()[0])
            field_count, = bs.read_u16()
            entry.data = [BINHelper.read_field(
                bs) for i in range(field_count)]
		*/
		this->entries.resize(entry_count);
		int entry_id = 0;
		for (auto& entry : this->entries) {
			entry.type = BINType(entry_types[entry_id]);
			reader.pad(4); // size
			entry.bin_hash = reader.readU64();
			auto field_count = reader.readU16();
			for (int i = 0; i < field_count; i++) {
				entry.data.emplace_back();
			}
		}
	}
	
	void BIN::write(std::ostringstream& outp_file) {
		throw std::runtime_error("Not implemented");
	}

    std::any BINHelper::readValue(BinaryReader& reader, BINType value_type)
	{
        std::any value;

        switch (value_type) {
        case BINType::Bool:
            value = reader.readBool();
            break;
        case BINType::I8:
            value = reader.readI8();
            break;
        case BINType::Flag:
        case BINType::U8:
            value = reader.readU8();
            break;
        case BINType::I16:
            value = reader.readI16();
            break;
        case BINType::U16:
            value = reader.readU16();
            break;
        case BINType::I32:
            value = reader.readI32();
            break;
        case BINType::Link:
        case BINType::Hash:
        case BINType::U32:
            value = reader.readU32();
            break;
        case BINType::I64:
            value = reader.readI64();
            break;
        case BINType::File:
        case BINType::U64:
            value = reader.readU64();
            break;
        case BINType::F32:
            value = reader.readF32();
            break;
        case BINType::Vec2:
            value = reader.readContainer<Container2, float, 2>();
            break;
        case BINType::Vec3:
            value = reader.readContainer<Container3, float, 3>();
            break;
        case BINType::Vec4:
            value = reader.readContainer<Container4, float, 4>();
            break;
        case BINType::Mtx4:
            value = reader.readMtx4();
            break;
        case BINType::RGBA:
            value = reader.readContainer<Container4, std::uint8_t, 4>();
            break;
        case BINType::String:
            value = reader.readStringSized16();
            break;

        case BINType::Embed:
        case BINType::Pointer:
            auto field = BINField();
            field.hash_type = reader.readU32();
            if (field.hash_type != 0) {
                field.type = value_type;
                reader.pad(4); // size
                
                auto fi_count = reader.readU16();
                std::vector<BINField> data;
                data.reserve(fi_count);
                for (int i = 0; i < fi_count; i++) {
                    data.emplace_back(readField(reader));
                }
                field.data = std::move(data);
            }
            else {
                field.data = nullptr;
            }
            //value = field;
            break;
        }

        return value;
	}

    BINField BINHelper::readField(BinaryReader& reader)
    {
        BINField field{};
        field.bin_hash = reader.readU32();
        field.type = BINType(reader.readU8());
        if (field.type == BINType::List || field.type == BINType::List2) {
            field.value_type = BINType(reader.readU8());
            reader.pad(4); // size
            auto count = reader.readU32();

            std::vector<std::any> data;
            data.reserve(count);
            for (uint32_t i = 0; i < count; ++i) {
                data.emplace_back(readValue(reader, field.value_type));
            }

            field.data = std::move(data);
        }
        return field;
    }
}