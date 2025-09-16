#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <ranges>
#include "include/skn.hpp"
#include "include/wad.hpp"
#include "include/bin.hpp"


enum ReturnVal : char {
	ERROR = -1,
	CHAMPION = 0,
	SFX_OR_AUDIO = 1,
	SOMETHING_ELSE = 2,
	SKIN_HACK = 3,
	MAYBE_SKIN_HACK = 4
};


char main(int argc, char* argv) {
	ReturnVal r_val = ReturnVal::ERROR;

	if (argc != 1) {
		return r_val;
	}
	const std::string file_path = "C:\\Users\\GuiSai\\Desktop\\Irelia.wad.client";

	std::ifstream inpt_file{ file_path, std::ios::binary };
	std::stringstream wad_buffer;
	if (inpt_file) {
		wad_buffer << inpt_file.rdbuf();
		inpt_file.close();
	}
	else {
		std::cerr << "Couldn't load file.\n";
		return r_val;
	}
	RitoFile::WAD wad = RitoFile::WAD(wad_buffer);

	wad.read();
	auto unpacked_chunks = std::unordered_map<std::uint64_t, std::vector<char>>();
	unpacked_chunks.reserve(wad.chunks.size());

	int nonCharBin = 0;
	int charBin = 0;
	int otherFile = 0;
	int voSfxFile = 0;

	for (const auto& chunk : wad.chunks) {
		auto chunk_data = wad.read_data(chunk);
		auto extension = RitoFile::guess_extension(*chunk_data);

		if (extension == "bin") {
			std::string str(chunk_data->begin(), chunk_data->end());
			std::stringstream iss(str);
			RitoFile::BIN bin_file = RitoFile::BIN(iss);
			bin_file.read();

			if (bin_file.get([](const RitoFile::BINEntry& e) {
					return e.type == RitoFile::fnv1a("SkinCharacterDataProperties");
			}).data.empty()) {
				nonCharBin += 1;
				continue;
			}

			charBin += 1;
		}
		else if (extension == "bnk" || extension == "wpk") {
			voSfxFile += 1;
		}
		else {
			otherFile += 1;
		}
	}
	
	if (otherFile >= 0
		&& charBin == 0
		&& nonCharBin == 0
		&& voSfxFile > 0
		) {
		/* Anything that has just audio and/or other things that aren't bins */
		r_val = ReturnVal::SFX_OR_AUDIO;
	}
	else if (
		otherFile >= 1
		&& charBin == 0
		&& nonCharBin >= 0
		&& voSfxFile >= 0) {
		/* Anything that has bins and/or other files, but no champ bin*/
		r_val = ReturnVal::SOMETHING_ELSE;
	}
	else if (
		otherFile == 0
		&& charBin == 1
		&& nonCharBin == 0
		&& voSfxFile == 0)
	{
		/* Just a champion bin, thats sus owo */
		r_val = ReturnVal::MAYBE_SKIN_HACK;
	}
	else if (
		otherFile >= 0
		&& charBin >= 0
		&& nonCharBin >= 0
		&& voSfxFile >= 0) {
		/**/
	}
	 
	return r_val;
}