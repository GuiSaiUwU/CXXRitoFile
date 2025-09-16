#pragma once
#include "binary_stream.hpp"
#include "structs.hpp"
#include "hash.hpp"

#include <vector>
#include <map>
#include <memory> // std::unique_ptr
#include <format> // std::format
#include <algorithm> // std::search

/* Third Party */
/* Decompressing / Compressing libs */
#include "gzip/decompress.hpp"
#include "gzip/utils.hpp"
#include <zstd.h>


namespace RitoFile {
	class WadZstdDecompressor {
#ifdef RITOFILE_USE_TOC_FILE
		ZSTD_DCtx* dctx_;
		ZSTD_DDict* ddict_ = nullptr;
#endif // RITOFILE_USE_TOC_FILE

	public:
#ifdef RITOFILE_USE_TOC_FILE

		WadZstdDecompressor() {
			dctx_ = ZSTD_createDCtx();

			ddict_ = ZSTD_createDDict(nullptr, 1024);

			//ZSTD_DCtx_reset(dctx_, ZSTD_reset_session_only);
			ZSTD_DCtx_refDDict(dctx_, ddict_);

		}

		~WadZstdDecompressor() {
			if (ddict_) {
				ZSTD_freeDDict(ddict_);
				ddict_ = nullptr;
			}
			ZSTD_freeDCtx(dctx_);
		}

		void unwrap(const std::vector<char>& src, std::vector<char>& dest);
#endif // RITOFILE_USE_TOC_FILE

		std::unique_ptr<std::vector<char>> decompressZstd(const std::vector<char>& data, size_t decompressed_size);
		std::unique_ptr<std::vector<char>> decompressZstd(const std::vector<char>& data);
	};

	std::string guess_extension(const std::vector<char>& data);

#ifdef RITOFILE_USE_TOC_FILE
	struct WADSubChunk {
		std::int32_t compressedSize;
		std::int32_t uncompressedSize;
		std::uint64_t checksum;
	};
#endif // RITOFILE_USE_TOC_FILE


	enum WADCompressionType : char {
		RAW = 0,
		GZIP = 1,
		SATELLITE = 2,
		ZSTD = 3,
		ZSTD_CHUNKED = 4
	};

	struct WADChunk {
		std::string extension;
		std::uint64_t checksum, hash;
		std::uint32_t offset, compressed_size, decompressed_size, id;

		std::uint16_t subchunk_start, subchunk_count;
		WADCompressionType compression_type;
		bool duplicated;
	};

	class WAD {
		WadZstdDecompressor zstd;

	public:
		std::vector<WADChunk> chunks;

#ifdef RITOFILE_USE_TOC_FILE
		std::vector<WADSubChunk> subchunks;
		void initialize_subchunks(std::uint64_t hash);
		std::unique_ptr<std::vector<char>> decompressZstdChunkedChunk(const WADChunk& chunk, std::vector<char>& chunkData);
#endif // RITOFILE_USE_TOC_FILE

		std::uint8_t major, minor;
		BinaryReader reader;

		WAD(std::stringstream& inpt_file);
		void read();
		void write(std::ostringstream& outp_file);

		std::unique_ptr<std::vector<char>> read_data(const WADChunk& chunk);
	};

	static const std::map<std::vector<char>, std::string> signature_to_extension = {
		{{'O', 'g', 'g', 'S'}, "ogg"},
		{{0x00, 0x01, 0x00, 0x00}, "ttf"},
		{{0x1A, 0x45, static_cast<char>(0xDF), static_cast<char>(0xA3)}, "webm"},
		{{'t', 'r', 'u', 'e'}, "ttf"},
		{{'O', 'T', 'T', 'O', 0x00}, "otf"},
		{{'"', 'u', 's', 'e', ' ', 's', 't', 'r', 'i', 'c', 't', '"', ';'}, "min.js"},
		{{'<', 't', 'e', 'm', 'p', 'l', 'a', 't', 'e', ' '}, "template.html"},
		{{'<', '!', '-', '-', ' ', 'E', 'l', 'e', 'm', 'e', 'n', 't', 's', ' ', '-', '-', '>'}, "template.html"},
		{{'D', 'D', 'S', ' '}, "dds"},
		{{'<', 's', 'v', 'g'}, "svg"},
		{{'P', 'R', 'O', 'P'}, "bin"},
		{{'P', 'T', 'C', 'H'}, "bin"},
		{{'B', 'K', 'H', 'D'}, "bnk"},
		{{'r', '3', 'd', '2', 'M', 'e', 's', 'h'}, "scb"},
		{{'r', '3', 'd', '2', 'a', 'n', 'm', 'd'}, "anm"},
		{{'r', '3', 'd', '2', 'c', 'a', 'n', 'm'}, "anm"},
		{{'r', '3', 'd', '2', 's', 'k', 'l', 't'}, "skl"},
		{{'r', '3', 'd', '2'}, "wpk"},
		{{0x33, 0x22, 0x11, 0x00}, "skn"},
		{{'P', 'r', 'e', 'L', 'o', 'a', 'd', 'B', 'u', 'i', 'l', 'd', 'i', 'n', 'g', 'B', 'l', 'o', 'c', 'k', 's', ' ', '=', ' ', '{'}, "preload"},
		{{0x1B, 'L', 'u', 'a', 'Q', 0x00, 0x01, 0x04, 0x04}, "luabin"},
		{{0x1B, 'L', 'u', 'a', 'Q', 0x00, 0x01, 0x04, 0x08}, "luabin64"},
		{{0x02, 0x3D, 0x00, 0x28}, "troybin"},
		{{'[', 'O', 'b', 'j', 'e', 'c', 't', 'B', 'e', 'g', 'i', 'n', ']'}, "sco"},
		{{'O', 'E', 'G', 'M'}, "mapgeo"},
		{{'T', 'E', 'X', 0x00}, "tex"},
		{{'R', 'W'}, "wad"}
	};
}