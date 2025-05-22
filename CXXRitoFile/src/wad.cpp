#include "wad.hpp"
#include <iostream>

namespace RitoFile {
	std::unique_ptr<std::vector<char>> WadZstdDecompressor::decompressZstd(const std::vector<char>& data) {
		size_t decompressed_size = ZSTD_getFrameContentSize(data.data(), data.size());
		if (decompressed_size == ZSTD_CONTENTSIZE_ERROR || decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN) {
			throw std::runtime_error("Invalid ZSTD frame content size");
		}
		auto decompressed_data = std::make_unique<std::vector<char>>(decompressed_size);

		size_t result = ZSTD_decompress(decompressed_data->data(), decompressed_size, data.data(), data.size());
		if (ZSTD_isError(result)) {
			throw std::runtime_error(std::format("ZSTD decompression failed: {}", ZSTD_getErrorString(ZSTD_getErrorCode(result))));
		}

		return decompressed_data;
	}

	std::unique_ptr<std::vector<char>> WadZstdDecompressor::decompressZstd(const std::vector<char>& data, size_t decompressed_size) {
		auto decompressed_data = std::make_unique<std::vector<char>>(decompressed_size);
		
		size_t result = ZSTD_decompress(decompressed_data->data(), decompressed_size, data.data(), data.size());
		if (ZSTD_isError(result)) {
			throw std::runtime_error(std::format("ZSTD decompression failed: {}", ZSTD_getErrorString(ZSTD_getErrorCode(result))));
		}

		return decompressed_data;
	}

	std::string guess_extension(const std::vector<char>& data) {
		if (data.size() >= 8 &&
			data[4] == static_cast<char>(0xC3) && data[5] == 0x4F &&
			data[6] == static_cast<char>(0xFD) && data[7] == 0x22) {
			return "skl";
		}

		for (const auto& [signature, extension] : signature_to_extension) {
			if (data.size() >= signature.size() &&
				std::equal(signature.begin(), signature.end(), data.begin())) {
				return extension;
			}
		}

		return "";
	}

	WAD::WAD(std::ifstream& inpt_file): reader(inpt_file) {
		if (!inpt_file.good()) {
			throw std::runtime_error("Stream not good");
		}

		this->major = 0;
		this->minor = 0;
	}

	void WAD::read() {
		auto signature = reader.readString(2);

		if (!(signature == "RW")) {
			throw std::runtime_error(std::format("Invalid signature {}", signature));
		}
		this->major = reader.readU8();
		this->minor = reader.readU8();

		if (this->major > 3) {
			throw new std::runtime_error(std::format("Unsupported version {}", this->major));
		}

		if (this->major == 2) {
			reader.pad(1); // ecdsa_len
			reader.pad(83); // ???
			reader.pad(4); // wad_checksum
		}
		else if (this->major == 3) {
			reader.pad(256);
			reader.pad(8); // wad_checksum (one u64)
		}
		if (major == 1 || major == 2) {
			reader.pad(4); // toc_start_offset, toc_file_entry_size (two u16)
		}
		const auto chunk_count = reader.readU32();
		this->chunks.resize(chunk_count);

		for (std::uint32_t i = 0; i < chunk_count; i++)
		{
			auto& chunk = this->chunks[i];
			chunk.id = i;
			//chunk.hash = std::format("{:x}", reader.readU64());
			chunk.hash = reader.readU64();
			chunk.offset = reader.readU32();
			chunk.compressed_size = reader.readU32();
			chunk.decompressed_size = reader.readU32();
			auto type_subChunkCount = reader.readU8();

			chunk.compression_type = WADCompressionType(type_subChunkCount & 0xF);
			chunk.duplicated = reader.readBool();
			chunk.subchunk_start = reader.readU16();
			chunk.subchunk_count = type_subChunkCount >> 4;
			chunk.checksum = major >= 2 ? reader.readU64() : 0;
		}
	}

	void WAD::write(std::ostringstream& outp_file) {
		throw std::runtime_error("Not implemented");
	}


	std::unique_ptr<std::vector<char>> WAD::read_data(const WADChunk& chunk)
	{
		reader.seek(chunk.offset);

		auto raw = std::make_unique<std::vector<char>>();
		reader.readBuffered<char>(*raw, chunk.compressed_size);

		auto data = std::make_unique<std::vector<char>>();
		std::string gzip_output;

		switch (chunk.compression_type)
		{
		case WADCompressionType::RAW:
			*data = std::move(*raw);
			break;

		case WADCompressionType::GZIP:
			gzip_output = gzip::decompress(raw->data(), raw->size());
			data->assign(gzip_output.begin(), gzip_output.end());
			break;

		case WADCompressionType::SATELLITE:
			/* Satellite is not supported */
			*data = std::move(*raw);
			break;

		case WADCompressionType::ZSTD:
			data = zstd.decompressZstd(*raw);
			break;

		case WADCompressionType::ZSTD_CHUNKED:
#ifdef RITOFILE_USE_TOC_FILE
			[[unlikely]]
			if (this->subchunks.size() == 0) {
				throw std::runtime_error("Trying to unpack ZSTD_CHUNKED without having subchunks initialized");
			}
			try {
				data = std::move(decompressZstdChunkedChunk(chunk, *raw));
			}
			catch (std::runtime_error) {
				*data = std::move(*raw);
			}
#else
			try {
				data = zstd.decompressZstd(*raw, chunk.decompressed_size);
			}
			catch (std::runtime_error) {
				*data = std::move(*raw);
			}
#endif
			break;
		}

		return data;
	}

#ifdef RITOFILE_USE_TOC_FILE
	void WAD::initialize_subchunks(std::uint64_t hash) {
		for (const auto& chunk : this->chunks) {
			if (chunk.hash == hash) {
				auto data = read_data(chunk);
				if (data->size() % sizeof(WADSubChunk) != 0) {
					throw std::runtime_error("Raw data size is not a multiple of WADSubChunk size");
				}

				this->subchunks.resize(data->size() / sizeof(WADSubChunk));
				std::memcpy(this->subchunks.data(), data->data(), data->size());
				for (const auto& subchunk : this->subchunks) {
				}
			}
		}

		if (subchunks.empty()) {
			throw std::runtime_error("Couldn't find toc entry");
		}
	}

	void WadZstdDecompressor::unwrap(const std::vector<char>& src, std::vector<char>& dest) {
		std::size_t result = ZSTD_decompressDCtx(dctx_,
			dest.data(), dest.size(),
			src.data(), src.size()
		);

		if (ZSTD_isError(result)) {
			throw std::runtime_error("ZSTD error: " + std::string(ZSTD_getErrorName(result)));
		}
	}

	std::unique_ptr<std::vector<char>> WAD::decompressZstdChunkedChunk(const WADChunk& chunk, std::vector<char>& chunk_data) {
		auto decompressedData = std::make_unique<std::vector<char>>(chunk.decompressed_size);

		size_t rawOffset = 0;
		size_t decompressedOffset = 0;

		for (size_t i = 0; i < chunk.subchunk_count; ++i) {
			const auto& subchunk = this->subchunks[chunk.subchunk_start + i];

			std::vector<char> src(
				chunk_data.begin() + rawOffset,
				chunk_data.begin() + rawOffset + subchunk.compressedSize
			);

			std::vector<char> destSlice(subchunk.uncompressedSize);

			try {
				zstd.unwrap(src, destSlice);

				std::copy(destSlice.begin(), destSlice.end(),
					decompressedData->begin() + decompressedOffset);
			}
			catch (const std::exception&) {
				std::copy(src.begin(), src.end(),
					decompressedData->begin() + decompressedOffset);
			}

			rawOffset += subchunk.compressedSize;
			decompressedOffset += subchunk.uncompressedSize;
		}

		return decompressedData;
	}

#endif // RITOFILE_USE_TOC_FILE
}
