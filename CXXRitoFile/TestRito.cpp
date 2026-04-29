#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <ranges>
#include "include/skn.hpp"
#include "include/wad.hpp"
#include "include/bin.hpp"
#include "include/tex.hpp"
#include "include/anm.hpp"
#include "include/mapgeo.hpp"


#define LOCK_AND_RETURN(v) \
	std::cin.get(); \
	return v

namespace fs = std::filesystem;
/* std::cerr for debugs and errors uwu */
/* std::cout for infos owo */

template <typename T>
T ritofile_cast(const std::any& data, const char* file, int line) {
	try {
		return std::any_cast<T>(data);
	}
	catch (...) {
		std::cerr << "Cast failed: expected type '" << typeid(T).name()
			<< "' from any (" << data.type().name() << ")"
			<< " at " << file << ":" << line << "\n";
		throw;
	}
}

#define cast(type, val) ritofile_cast<type>(val, __FILE__, __LINE__)

void test_wad() {
	//const std::string file_path = "C:\\Users\\GuiSai\\Desktop\\TestCRito\\Strawberry_Briar.wad.client";
	//const std::string file_path = "C:\\Riot Games\\League of Legends\\Game\\DATA\\FINAL\\Champions\\Darius.wad.client";
	//const std::string file_path = "C:\\Riot Games\\League of Legends\\Game\\DATA\\FINAL\\Maps\\Shipping\\Map11.wad.client";
	const std::string file_path = "C:\\Users\\GuiSai\\Desktop\\cslol-manager\\installed\\TFTMapsSkins\\WAD\\Map22.wad.client";
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
			std::cout << std::format("Couldn't guess: {:x}", chunk.hash) << "\n";
		}
	}

	std::cout << "End" << "\n";
}

static inline RitoFile::SKN read_skn(const std::string& file_path) {
	std::ifstream inpt_file{ file_path, std::ios::binary };
	std::stringstream skn_buffer;
	if (inpt_file) {
		skn_buffer << inpt_file.rdbuf();
		inpt_file.close();
	}
	else {
		std::cout << "Couldn't load file.\n";
	}
	RitoFile::SKN mesh = RitoFile::SKN(skn_buffer);
	mesh.read();

	return mesh;
}

void test_bin() {
	const std::string file_path = "C:\\Users\\GuiSai\\Desktop\\TestCRito\\skin17.bin";
	std::ifstream inpt_file{ file_path, std::ios::binary };
	std::stringstream bin_buffer;
	if (inpt_file) {
		bin_buffer << inpt_file.rdbuf();
		inpt_file.close();
	}
	else {
		std::cout << "Couldn't load file.\n";
		return;
	}

	RitoFile::BIN bin_file{ bin_buffer };
	bin_file.read();
	std::cout << std::format("Size: {}.\n", bin_file.links.size());

	std::vector<RitoFile::BINField> skinCharacterDataProperties{};

	{
		auto x = bin_file.get_items([](const RitoFile::BINEntry& e) {
			return e.type == RitoFile::fnv1a("SkinCharacterDataProperties");
		});

		if (!x.empty()) {
			skinCharacterDataProperties = std::any_cast<std::vector<RitoFile::BINField>>(x.at(0));
		}
		else {
			std::cerr << "Couldn't find skinCharacterDataProperties";
		}
	}

	for (auto& sub_entry : skinCharacterDataProperties) {
		if (sub_entry.hash_type != RitoFile::fnv1a("SkinMeshDataProperties") || sub_entry.type != RitoFile::BINType::Embed) {
			continue;
		}


		for (auto& x : cast(std::vector<RitoFile::BINField>, sub_entry.data)) {
			if (x.type == RitoFile::BINType::String) {
				std::cout << "Str Val: " << cast(std::string, x.data) << "\n";
			}
		}
	}

}

void parse_skn(const std::string& file_path) {
	RitoFile::SKN mesh = read_skn(file_path);
	std::cerr << "SKN read successfully!" << "\n";
	std::cout << "Version: " << mesh.major << "." << mesh.minor << "\n" << "\n";
	std::cout << "Submeshes: " << mesh.submeshes.size() << "\n";
	int total_indices = 0;
	int total_vertices = 0;

	for (int i = 0; i < mesh.submeshes.size(); i++) {
		const auto& submesh = mesh.submeshes.at(i);
		total_indices += submesh.index_count;
		total_vertices += submesh.vertex_count;
		std::cout << std::format("[{:>2}] {:<64} Indices: {} Vertices: {}\n", i, submesh.name, submesh.index_count, submesh.vertex_count);
	}

	std::cout << "\n" << "Total Indices: " << total_indices << "\n";
	std::cout << "Total Vertices: " << total_vertices << "\n";
	std::cout << std::format("{:_^64}\n", "");
	
#ifdef _DEBUG
	std::cout << "Writing SKN...\n";
	std::ostringstream outp_file{};
	mesh.write(outp_file);
	std::ofstream out_file{ "C:\\Users\\GuiSai\\Desktop\\TestCRito\\sett_skin66_2.skn", std::ios::binary };
	out_file.write(outp_file.str().c_str(), outp_file.str().size());

#endif // DEBUG
}

void test_anm() {
	const std::string file_path = "C:\\Users\\GuiSai\\Desktop\\TestCRito\\sett_attack1.anm";
	std::ifstream inpt_file{ file_path, std::ios::binary };
	std::stringstream anm_buffer;
	if (inpt_file) {
		anm_buffer << inpt_file.rdbuf();
		inpt_file.close();
	}
	else {
		std::cout << "Couldn't load file.\n";
		return;
	}
	RitoFile::ANM anm_file{ anm_buffer };
	anm_file.read();
	std::cout << std::format("ANM Version: {}.\n", anm_file.version);
	std::cout << std::format("ANM Tracks: {}.\n", anm_file.tracks.size());
}

void test_mapgeo() {
	const std::string file_path = "C:\\Users\\GuiSai\\Desktop\\TestCRito\\base_a.mapgeo";
	std::ifstream inpt_file{ file_path, std::ios::binary };
	std::stringstream mapgeo_buffer;

	if (inpt_file) {
		mapgeo_buffer << inpt_file.rdbuf();
		inpt_file.close();
	}
	else {
		std::cout << "Couldn't load file.\n";
		return;
	}

	RitoFile::MAPGEO mapgeo_file{ mapgeo_buffer };
	mapgeo_file.read();
	std::cout << std::format("MAPGEO Version: {}.\n", mapgeo_file.version);
}

void test_tex() {
	std::string file_path = "C:\\Users\\GuiSai\\Desktop\\TestCRito\\test_tex.tex";
	std::ifstream inpt_file{ file_path, std::ios::binary };	
	std::stringstream tex_buffer;
	if (inpt_file) {
		tex_buffer << inpt_file.rdbuf();
		inpt_file.close();
	}
	else {
		std::cout << "Couldn't load file.\n";
		return;
	}
	RitoFile::TEX tex_file{ tex_buffer };
	tex_file.read();
	
	std::string output_path = "C:\\Users\\GuiSai\\Desktop\\TestCRito\\test_tex_out.tex";
	std::ofstream outp_file{ output_path, std::ios::binary };
	if (outp_file) {
		std::ostringstream out_buffer;
		tex_file.write(out_buffer);
		outp_file.write(out_buffer.str().c_str(), out_buffer.str().size());
		outp_file.close();
		std::cout << "TEX written successfully!\n";
	}
	else {
		std::cout << "Couldn't write file.\n";
		return;
	}

	std::string dds_output_path = "C:\\Users\\GuiSai\\Desktop\\TestCRito\\test_tex_out.dds";
	std::ofstream dds_outp_file{ dds_output_path, std::ios::binary };
	if (dds_outp_file) {
		std::ostringstream out_buffer;
		tex_file.to_dds(out_buffer);
		dds_outp_file.write(out_buffer.str().c_str(), out_buffer.str().size());
		dds_outp_file.close();
		std::cout << "DDS written successfully!\n";
	}
	else {
		std::cout << "Couldn't write file.\n";
		return;
	}
}


int main(int argc, char* argv[]) {
#ifdef _DEBUG
	//test_wad();
	//test_bin();
	//test_anm();
	//test_mapgeo();
	test_tex();
#endif

#ifndef _DEBUG
	if (argc < 2) {
		std::cerr << "No file path provided.\n";
		LOCK_AND_RETURN(-1);
	}

	std::cerr << "Working File: " << argv[1] << "\n";

	std::string file_path = argv[1];

	std::error_code ec;
	if (!fs::is_regular_file(fs::path(file_path), ec)) {
		std::cerr << "Not an file!\n";
		LOCK_AND_RETURN(-1);
	}

	if (ec) {
		std::cerr << "Error while checking file: " << ec.message() << "\n";
		LOCK_AND_RETURN(-1);
	}

	if (file_path.ends_with(".wad.client")) {
		std::cerr << "Not implemented.\n";
		LOCK_AND_RETURN(-1);
	}
	else if (file_path.ends_with(".skn")) {
		parse_skn(file_path);
		LOCK_AND_RETURN(0);
	}

#endif
	return 0;
}