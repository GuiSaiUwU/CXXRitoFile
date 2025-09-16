#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <ranges>
#include "include/skn.hpp"
#include "include/wad.hpp"
#include "include/bin.hpp"


void test_hack() {
	const std::string file_path = "C:\\Users\\GuiSai\\Desktop\\TestCRito\\Strawberry_Briar.wad.client";
	//const std::string file_path = "C:\\Riot Games\\League of Legends\\Game\\DATA\\FINAL\\Champions\\Darius.wad.client";
	//const std::string file_path = "C:\\Riot Games\\League of Legends\\Game\\DATA\\FINAL\\Maps\\Shipping\\Map11.wad.client";

	std::ifstream inpt_file{ file_path, std::ios::binary };
	std::stringstream wad_buffer;
	if (inpt_file) {
		wad_buffer << inpt_file.rdbuf();
		inpt_file.close();
	}
	else {
		std::cout << "Couldn't load file.\n";
		return;
	}
	RitoFile::WAD wad = RitoFile::WAD(wad_buffer);

	wad.read();
	//const std::string subchunk_path = "data/final/champions/darius.wad.subchunktoc";

	//std::string_view subchunk_path = "data/final/champions/strawberry_briar.wad.subchunktoc";
	//wad.initialize_subchunks(xxh64(subchunk_path.data(), subchunk_path.size()));

	auto unpacked_chunks = std::unordered_map<std::uint64_t, std::vector<char>>();
	unpacked_chunks.reserve(wad.chunks.size());

	for (const auto& chunk : wad.chunks) {
		auto chunk_data = wad.read_data(chunk);
		auto extension = RitoFile::guess_extension(*chunk_data);

		if (extension == "") {
			std::string str(chunk_data->begin(), chunk_data->end());
			std::stringstream iss(str);
			RitoFile::BIN bin = RitoFile::BIN(iss);
		}
	}

	std::cout << "End" << "\n";
}