#pragma once
#include "binary_stream.hpp"
#include "structs.hpp"
#include "hash.hpp"
#include <vector>
#include <variant>
#include <array>
#include <format>
#include <utility>


namespace RitoFile {
	enum SKNVertexType : std::uint8_t {
		BASIC,
		COLOR,
		TANGENT
	};

	struct SKNVertex {
		Container4<float> tangent; // <= Only if SKNVertexType::TANGENT
		Container4<float> weights;
		Container3<float> normal;
		Container3<float> pos;
		Container2<float> uv;
		Container4<std::uint8_t> influences;
		Container4<std::uint8_t> color; // <= Only if SKNVertexType::TANGENT or SKNVertexType::COLOR
	};

	struct SKNSubmesh {
		std::string name;
		std::uint32_t bin_hash;
		std::uint32_t index_start, index_count;
		std::uint32_t vertex_start, vertex_count;

		// Default
		SKNSubmesh() :
			name("Base"), bin_hash(fnv1a("Base")),
			index_start(0), index_count(0),
			vertex_start(0), vertex_count(0)
		{
		}

		// For major 0
		SKNSubmesh(const std::uint32_t& index_count, const std::uint32_t& vertex_count) :
			name("Base"), bin_hash(fnv1a("Base")),
			index_start(0), index_count(index_count),
			vertex_start(0), vertex_count(vertex_count) {
		}

		// All Fields
		SKNSubmesh(const std::string& name, const std::uint32_t& bin_hash, const std::uint32_t& index_start, const std::uint32_t& index_count, const std::uint32_t& vertex_start, const std::uint32_t& vertex_count)
			: name(name), bin_hash(bin_hash),
			index_start(index_start), index_count(index_count),
			vertex_start(vertex_start), vertex_count(vertex_count) {
		}
	};


	/* -------------------------- */
	class SKN {
	public:
		std::vector<SKNVertex> vertices;
		std::vector<SKNSubmesh> submeshes;
		std::vector<std::uint16_t> indices;
		Container2<Container3<float>> bounding_box;
		std::pair<Container3<float>, float> bounding_sphere;
		std::uint16_t major, minor;
		std::uint32_t flags, vertex_size;
		SKNVertexType vertex_type;
		BinaryReader reader;

		SKN(std::stringstream& inpt_file);
		void read();
		void write(std::ostringstream& outp_file);
	};

};