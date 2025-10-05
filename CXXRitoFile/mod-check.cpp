#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <ranges>
#include "include/skn.hpp"
#include "include/wad.hpp"
#include "include/bin.hpp"


std::string getChampionNameFromPath(const std::string& filePath) {
	size_t lastSeparator = filePath.find_last_of("/\\");

	std::string filename = (lastSeparator != std::string::npos) ? 
		filePath.substr(lastSeparator + 1) : filePath;
	
	size_t wadPos = filename.find(".wad.client");
	if (wadPos != std::string::npos) {
		return filename.substr(0, wadPos);
	}
	
	size_t dotPos = filename.find_first_of('.');
	if (dotPos != std::string::npos) {
		return filename.substr(0, dotPos);
	}
	
	// If no extension found, return the whole filename
	return filename;
}

std::vector<std::string> splitStr(std::string s, std::string delimiter) {
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	std::string token;
	std::vector<std::string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
		token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(s.substr(pos_start));
	return res;
}

enum ReturnVal : int {
	ERROR = -1,
	CHAMPION = 0,
	SFX_OR_AUDIO = 1,
	SKIN_HACK = 2,
	SOMETHING_ELSE = 3,
	MAYBE_SKIN_HACK = 4
};


int main(int argc, char* argv[]) {
	ReturnVal r_val = ReturnVal::ERROR;

#ifndef DEBUGUWU
	if (argc != 2) {
		return r_val;
	}
	const std::string file_path(argv[1]);

#else
	const std::string file_path = "C:\\Users\\GuiSai\\Desktop\\Milio.wad.client";
#endif


	std::ifstream inpt_file{ file_path, std::ios::binary };
	std::stringstream wad_buffer;
	if (inpt_file) {
		wad_buffer << inpt_file.rdbuf();
		inpt_file.close();
	}
	else {
		std::cout << "{UNKNOWN}";
		return r_val;
	}

	RitoFile::WAD wad = RitoFile::WAD(wad_buffer);
	wad.read();

	auto unpacked_chunks = std::unordered_map<std::uint64_t, std::vector<char>>();
	unpacked_chunks.reserve(wad.chunks.size());

	std::string championName = getChampionNameFromPath(file_path);
	//std::cout << "Champion name: " << championName << std::endl;
	
	int nonCharBin = 0;
	int charBin = 0;
	int otherFile = 0;
	int voSfxFile = 0;

	bool isSkinHack = false;
	for (const auto& chunk : wad.chunks) {
		auto chunk_data = wad.read_data(chunk);
		auto extension = RitoFile::guess_extension(*chunk_data);

		if (extension == "bin") {
			std::string str(chunk_data->begin(), chunk_data->end());
			std::stringstream iss(str);
			RitoFile::BIN bin_file = RitoFile::BIN(iss);
			bin_file.read();

			auto skinCharData = bin_file.get([](const RitoFile::BINEntry& e) {
				return e.type == RitoFile::fnv1a("SkinCharacterDataProperties");
			});

			if (skinCharData.data.empty()) {
				nonCharBin += 1;
				continue;
			}
			
			charBin += 1;

			/* Check for skin stuff */
			short skinId = -1;
			for (short x = 0; x < 200; x++) {
				if (skinCharData.bin_hash == RitoFile::fnv1a(std::format("Characters/{}/Skins/Skin{}", championName, x))) {
					skinId = x;
					break;
				}
			}

			if (skinId == -1) {
				continue;
			}

			// We just check if the loadscreen is an skin one
			for (auto const& char_prop : skinCharData.data) {
				if (char_prop.bin_hash == RitoFile::fnv1a("loadscreen")) {
					if (char_prop.type == RitoFile::Embed) {
						auto imageDatas = std::any_cast<std::vector<RitoFile::BINField>>(char_prop.data);
						for (auto const& imageData : imageDatas) {
							if (imageData.type == RitoFile::String) {
								auto loadingScreenPath = std::any_cast<std::string>(imageData.data);
								//std::cout << loadingScreenPath;
								if (static_cast<int>(std::count(loadingScreenPath.begin(), loadingScreenPath.end(), '/')) == 5) {
									// Ex: ASSETS/Characters/Irelia/Skins/Skin45/IreliaLoadScreen_45.tex
									auto splittedString = splitStr(loadingScreenPath, "/");
									std::string skinNumberInLoadingScreen = splittedString.at(4);
									std::transform(skinNumberInLoadingScreen.begin(), skinNumberInLoadingScreen.end(), skinNumberInLoadingScreen.begin(),
										[](unsigned char c) { return std::tolower(c); });

									if (skinNumberInLoadingScreen != std::format("skin{:02}", skinId) && skinNumberInLoadingScreen != "base") {
										isSkinHack = true;
									}
								}
							}
						}
					}
				}
			}
		}
		else if (extension == "bnk" || extension == "wpk") {
			voSfxFile += 1;
		}
		else {
			otherFile += 1;
		}
	}
	
	if (isSkinHack && otherFile == 0 && nonCharBin == 0 && voSfxFile == 0 && charBin == 1
	){
		// Skin hack flag & just charBin
		r_val = ReturnVal::SKIN_HACK;
		std::cout << "{PIRATE}";
	}
	else if (otherFile == 0
		&& charBin == 0
		&& nonCharBin == 0
		&& voSfxFile > 0
		) {
		/* Anything that has just audio and/or other things that aren't bins */
		r_val = ReturnVal::SFX_OR_AUDIO;
		std::cout << "{AUDIO}";
	}
	else if (
		otherFile >= 1
		&& charBin == 0
		&& nonCharBin >= 0
		&& voSfxFile >= 0) {
		/* Anything that has bins and/or other files, but no champ bin*/
		r_val = ReturnVal::SOMETHING_ELSE;
		std::cout << "{UNKNOWN}";
	}
	else if (
		otherFile == 0
		&& charBin == 1
		&& nonCharBin == 0
		&& voSfxFile == 0)
	{
		/* Just a champion bin, thats sus owo */
		r_val = ReturnVal::MAYBE_SKIN_HACK;
		std::cout << "{MAYBE-PIRATE}";
	}
	else if (
		otherFile >= 0
		&& charBin >= 0
		&& nonCharBin >= 0
		&& voSfxFile >= 0) {
		/**/
		r_val = ReturnVal::CHAMPION;
		std::cout << "{UNKNOWN}";
	}
	 
	return r_val;
}