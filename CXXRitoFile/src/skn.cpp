#include "skn.hpp"


namespace RitoFile {
	SKN::SKN(std::ifstream& inpt_file) : reader(inpt_file) {
		if (!inpt_file.good()) {
			throw std::runtime_error("Stream not good");
		}

		this->vertex_type = SKNVertexType::BASIC;
		this->major = 0;
		this->minor = 0;
		this->flags = 0;
		this->vertex_size = 0;
	}

	void SKN::read() {
#pragma region HeaderRead

		auto signature = reader.readU32();
		if (signature != 0x00112233) {
			throw std::runtime_error("Invalid SKN Signature");
		}

		this->major = reader.readU16();
		this->minor = reader.readU16();

		// major not in (0, 2, 4) and minor != 1
		const static std::array<int, 3> allowedMajors = { 0, 2, 4 };
		if (std::find(allowedMajors.begin(), allowedMajors.end(), major) == allowedMajors.end()
			&& minor != 1) {
			throw std::runtime_error(std::format("Invalid version {}.{}", major, minor));
		}

#pragma endregion

#pragma region SubmeshesAndCounts

		std::uint32_t index_count = 0;
		std::uint32_t vertex_count = 0;

		if (major == 0) {
			index_count = reader.readU32();
			vertex_count = reader.readU32();
			this->submeshes.push_back(
				SKNSubmesh{
					index_count,
					vertex_count
				}
			);
		}
		else { // has submeshes
			auto submesh_count = reader.readU32();
			this->submeshes.resize(submesh_count);
			for (auto& s : submeshes) {
				s.name = reader.readStringPadded(64);
				s.bin_hash = fnv1a_lower_cased(s.name);
				s.vertex_start = reader.readU32();
				s.vertex_count = reader.readU32();
				s.index_start = reader.readU32();
				s.index_count = reader.readU32();
			}

			if (major == 4) {
				this->flags = reader.readU32();
			}

			index_count = reader.readU32();
			vertex_count = reader.readU32();

			if (major == 4) {
				this->vertex_size = reader.readU32();
				this->vertex_type = SKNVertexType(reader.readU32());
				this->bounding_box = Container2<Container3<float>>(
					reader.readContainer<Container3, float, 3>(),
					reader.readContainer<Container3, float, 3>()
				);
				this->bounding_sphere = std::make_pair(
					reader.readContainer<Container3, float, 3>(),
					reader.readF32()
				);
			}
		}
		if (index_count % 3 != 0) {
			throw std::runtime_error(std::format("Invalid index_count {}", index_count));
		}

#pragma endregion

#pragma region IndicesAndVerticesRead

		std::vector<std::uint16_t> local_indices(index_count);
		reader.readBuffered<std::uint16_t>(local_indices, index_count * sizeof(std::uint16_t));

		this->indices.reserve(index_count);
		for (std::uint32_t i = 0; i < index_count; i += 3) {

			[[unlikely]]
			if (local_indices[i] == local_indices[i + 1] ||
				local_indices[i] == local_indices[i + 2] ||
				local_indices[i + 1] == local_indices[i + 2]) {
				continue; // Zac SKN for some reason has bad faces
			}
			this->indices.push_back(local_indices[i]);
			this->indices.push_back(local_indices[i + 1]);
			this->indices.push_back(local_indices[i + 2]);
		}

		this->vertices.resize(vertex_count);
		for (auto& vertex : vertices) {
			vertex.pos = reader.readContainer<Container3, float, 3>();
			vertex.influences = reader.readContainer<Container4, std::uint8_t, 4>();
			vertex.weights = reader.readContainer<Container4, float, 4>();
			vertex.normal = reader.readContainer<Container3, float, 3>();
			vertex.uv = reader.readContainer<Container2, float, 2>();
			if (this->vertex_type == SKNVertexType::COLOR || this->vertex_type == SKNVertexType::TANGENT) {
				vertex.color = reader.readContainer<Container4, std::uint8_t, 4>();
				if (this->vertex_type == SKNVertexType::TANGENT) {
					vertex.tangent = reader.readContainer<Container4, float, 4>();
				}
			}
		}

#pragma endregion

	}

	void SKN::write(std::ostringstream& outp_file) {
		BinaryWriter writer{ outp_file };

#pragma region HeaderWrite
		
		writer.writeU32(0x00112233);
		if (this->major >= 4) {
			writer.writeU16(4);
			writer.writeU16(1);
		}
		else {
			writer.writeU16(1);
			writer.writeU16(1);
		}

#pragma endregion

#pragma region SubmeshWrite

		writer.writeU32(this->submeshes.size());
		for (const auto& submesh : this->submeshes) {
			writer.writeStringPadded(submesh.name, 64);
			writer.writeU32(submesh.vertex_start);
			writer.writeU32(submesh.vertex_count);
			writer.writeU32(submesh.index_start);
			writer.writeU32(submesh.index_count);
		}
		if (this->major >= 4) {
			writer.writeU32(this->flags);
		}

#pragma endregion

#pragma region ExtraInfoWrite
		
		writer.writeU32(this->indices.size());
		writer.writeU32(this->vertices.size());
		if (this->major >= 4) {
			writer.writeU32(this->vertex_size);
			writer.writeU32(this->vertex_type);
			writer.writeContainer<Container3, float, 3>(this->bounding_box.x);
			writer.writeContainer<Container3, float, 3>(this->bounding_box.y);
			writer.writeContainer<Container3, float, 3>(this->bounding_sphere.first);
			writer.writeF32(this->bounding_sphere.second);
		}

#pragma endregion

#pragma region IndicesAndVerticesWrite
		
		for (const auto& index : this->indices) {
			writer.writeU16(index);
		}

		for (const auto& vertices : this->vertices) {
			writer.writeContainer<Container3, float, 3>(vertices.pos);
			writer.writeContainer<Container4, std::uint8_t, 4>(vertices.influences);
			writer.writeContainer<Container4, float, 4>(vertices.weights);
			writer.writeContainer<Container3, float, 3>(vertices.normal);
			writer.writeContainer<Container2, float, 2>(vertices.uv);
			if (this->vertex_type == SKNVertexType::COLOR || this->vertex_type == SKNVertexType::TANGENT) {
				writer.writeContainer<Container4, std::uint8_t, 4>(vertices.color);
				if (this->vertex_type == SKNVertexType::TANGENT) {
					writer.writeContainer<Container4, float, 4>(vertices.tangent);
				}
			}
		}

#pragma endregion

	}
}