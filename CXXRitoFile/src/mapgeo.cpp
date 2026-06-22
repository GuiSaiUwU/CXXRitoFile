#include "mapgeo.hpp"
#include <format>
#include <array>
#include <algorithm>
#include <utility>
#include <limits>
#include <cmath>
#include <cstring>


namespace RitoFile {
	MAPGEO::MAPGEO(std::stringstream& inpt_file)
		: reader(inpt_file) {
		if (!inpt_file.good()) {
			throw std::runtime_error("Input file stream is not good for reading MAPGEO.");
		}
	}

	const static std::array<std::uint32_t, 10> valid_versions = {
		5, 6, 7, 9, 11, 12, 13, 14, 15, 17
	};

	void MAPGEO::read() {
		this->signature = reader.readString(4);

		if (this->signature != "OEGM") {
			throw std::runtime_error(std::format("Wrong version {}", this->signature));
		}

		this->version = reader.readU32();

		// if self.version not in (5, 6, 7, 9, 11, 12, 13, 14, 15, 17):
		if (std::find(std::begin(valid_versions), std::end(valid_versions), this->version) == std::end(valid_versions)) {
			throw std::runtime_error(std::format("Unsupported MAPGEO version {}", this->version));
		}

		bool use_seperate_point_lights = false;
		if (this->version < 7) {
			use_seperate_point_lights = reader.readBool();
		}

		// Texture overrides
		if (this->version >= 17) {
			std::uint32_t texture_override_count = reader.readU32();
			this->texture_overrides.resize(texture_override_count);
			for (auto& texture_override : this->texture_overrides) {
				texture_override.index = reader.readU32();
				texture_override.path = reader.readStringSized32();
			}
		}
		else {
			if (this->version >= 9) {
				MAPGEOTextureOverride texture_override;
				texture_override.index = 0;
				texture_override.path = reader.readStringSized32();
				this->texture_overrides.push_back(texture_override);

				if (this->version >= 11) {
					MAPGEOTextureOverride texture_override2;
					texture_override2.index = 1;
					texture_override2.path = reader.readStringSized32();
					this->texture_overrides.push_back(texture_override2);
				}
			}
		}

		// Vertex descriptions
		std::uint32_t vertex_description_count = reader.readU32();
		this->vertex_descriptions.resize(vertex_description_count);
		for (auto& vertex_description : this->vertex_descriptions) {
			vertex_description.usage = static_cast<MAPGEOVertexUsage>(reader.readU32());
			std::uint32_t element_count = reader.readU32();
			vertex_description.elements.resize(element_count);

			std::vector<std::uint32_t> unpacked_u32s;
			reader.readBuffered(unpacked_u32s, element_count * 2 * sizeof(std::uint32_t));
			
			std::size_t element_id = 0;
			for (auto& element : vertex_description.elements) {
				element.name = static_cast<MAPGEOVertexElementName>(unpacked_u32s[element_id * 2]);
				element.format = static_cast<MAPGEOVertexElementFormat>(unpacked_u32s[(element_id * 2 )+ 1]);
				element_id++;
			}
			reader.pad(8 * (15 - element_count));  // pad empty vertex descriptions
		}

		/*
		# vertex buffers 
		# -> can not read now because need vertex descriptions 
		# -> read vertex buffer's offset instead
		*/
		std::uint32_t vertex_buffer_count = reader.readU32();
		std::vector<std::size_t> vertex_buffer_offsets(vertex_buffer_count);
		std::vector<std::uint32_t> vertex_buffer_sizes(vertex_buffer_count); // (C++ only) Store the size too
		for (std::uint32_t i = 0; i < vertex_buffer_count; i++) {
			if (this->version >= 13) {
				reader.pad(1);  // layer
			}
			vertex_buffer_sizes[i] = reader.readU32(); // size = byte count
			vertex_buffer_offsets[i] = reader.tell();
			reader.pad(vertex_buffer_sizes[i]);
		}

		// Index buffers
		std::uint32_t index_buffer_count = reader.readU32();
		std::vector<std::vector<std::uint16_t>> index_buffers(index_buffer_count);

		for (std::uint32_t i = 0; i < index_buffer_count; i++) {
			if (this->version >= 13) {
				reader.pad(1);  // layer
			}
			std::uint32_t index_buffer_size = reader.readU32(); // size = byte count
			reader.readBuffered(index_buffers[i], index_buffer_size);
		}

		// Read Models
		std::vector<std::vector<MAPGEOVertex>> cached_vertex_buffers;
		cached_vertex_buffers.resize(vertex_buffer_count);

		std::uint32_t model_count = reader.readU32();
		this->models.resize(model_count);
		for (std::uint32_t model_id = 0; model_id < model_count; model_id++) {
			MAPGEOModel& model = this->models[model_id];

			if (this->version < 12) {
				model.name = reader.readStringSized32();
			}
			else {
				model.name = std::format("Mapgeo_Instance_{}", model_id);
			}

			model.vertex_count = reader.readU32();
			model.vertex_buffer_count = reader.readU32();
			model.vertex_description_id = reader.readU32();

			reader.readBuffered(model.vertex_buffer_ids, model.vertex_buffer_count * sizeof(std::uint32_t));

			model.vertices.clear();
			model.vertices.resize(model.vertex_count);

			// Read and cache each referenced vertex buffer
			// This is so goofy wtf
			for (std::size_t i = 0; i < model.vertex_buffer_ids.size(); i++) {
				const std::uint32_t vertex_buffer_id = model.vertex_buffer_ids[i];

				if (vertex_buffer_id >= vertex_buffer_count) {
					continue;
				}

				const auto& vertex_description = this->vertex_descriptions.at(model.vertex_description_id + i);

				std::uint32_t vertex_stride = 0;
				for (const auto& element : vertex_description.elements) {
					vertex_stride += getElementSize(element.format);
				}

				std::uint32_t buffer_vertex_count = vertex_buffer_sizes[vertex_buffer_id] / vertex_stride;

				if (cached_vertex_buffers[vertex_buffer_id].empty()) {
					auto return_offset = reader.tell();
					reader.seek(vertex_buffer_offsets.at(vertex_buffer_id));

					cached_vertex_buffers[vertex_buffer_id].resize(buffer_vertex_count);
					for (std::uint32_t v = 0; v < buffer_vertex_count; v++) {
						cached_vertex_buffers[vertex_buffer_id][v] = MAPGEOVertexReader::readVertex(reader, vertex_description.elements);
					}

					reader.seek(return_offset);
				}

				std::uint32_t vertices_to_copy = std::min(model.vertex_count, buffer_vertex_count);

				const auto& vb_vertices = cached_vertex_buffers[vertex_buffer_id];
				for (std::uint32_t v = 0; v < vertices_to_copy; v++) {
					const auto& partial_vertex = vb_vertices[v];
					// Merge element values from this buffer into the model's vertex
					for (const auto& elem : vertex_description.elements) {
						model.vertices[v].value[elem.name] = partial_vertex.value.at(elem.name);
					}
				}
			}

			// Model indices
			model.index_count = reader.readU32();
			model.index_buffer_id = reader.readU32();
			if (model.index_buffer_id < index_buffers.size()) {
				model.indices = index_buffers.at(model.index_buffer_id);
			}

			if (this->version >= 13) {
				model.layer = static_cast<MAPGEOLayer>(reader.readU8());
			}

			if (this->version >= 15) {
				model.bucket_grid_hash = reader.readU32();
			}

			// Submeshes
			{
				std::uint32_t submesh_count = reader.readU32();
				model.submeshes.resize(submesh_count);
				for (auto& submesh : model.submeshes) {
					submesh.hash = reader.readU32();
					submesh.name = reader.readStringSized32();
					submesh.index_start = reader.readU32();
					submesh.index_count = reader.readU32();
					submesh.min_vertex = reader.readU32();
					submesh.max_vertex = reader.readU32();
				}
			}

			// Disable backface culling (not in version 5)
			if (this->version != 5) {
				model.disable_backface_culling = reader.readBool();
			}

			// Bounding box (min, max)

			Container3<float> bb_min;
			bb_min.x = reader.readF32(); bb_min.y = reader.readF32(); bb_min.z = reader.readF32();
			Container3<float> bb_max;
			bb_max.x = reader.readF32(); bb_max.y = reader.readF32(); bb_max.z = reader.readF32();
			model.bounding_box.x = bb_min;
			model.bounding_box.y = bb_max;


			model.matrix = reader.readMtx4();
			model.quality = static_cast<MAPGEOQuality>(reader.readU8());

			if (this->version >= 7 && this->version <= 12) {
				model.layer = static_cast<MAPGEOLayer>(reader.readU8());
			}

			// Render and bush flags
			if (this->version >= 11) {
				if (this->version < 14) {
					model.render = static_cast<MAPGEORender>(reader.readU8());
				} else {
					model.is_bush = reader.readBool();
					if (this->version < 16) {
						model.render = static_cast<MAPGEORender>(reader.readU8());
					} else {
						model.render = static_cast<MAPGEORender>(reader.readU16());
					}
				}
			}

			// Point light (version < 7 and separate point lights enabled)
			if (this->version < 7 && use_seperate_point_lights) {
				model.point_light.x = reader.readF32();
				model.point_light.y = reader.readF32();
				model.point_light.z = reader.readF32();
			}

			// Light probes (version < 9)
			if (this->version < 9) {
				for (int i = 0; i < 9; ++i) {
					model.light_probe.x[i] = reader.readF32();
				}
				for (int i = 0; i < 9; ++i) {
					model.light_probe.y[i] = reader.readF32();
				}
				for (int i = 0; i < 9; ++i) {
					model.light_probe.z[i] = reader.readF32();
				}
			}

			// Channels: baked_light
			model.baked_light.path = reader.readStringSized32();
			model.baked_light.scale.x = reader.readF32();
			model.baked_light.scale.y = reader.readF32();
			model.baked_light.offset.x = reader.readF32();
			model.baked_light.offset.y = reader.readF32();

			// Stationary light (version >= 9)
			if (this->version >= 9) {
				model.stationary_light.path = reader.readStringSized32();
				model.stationary_light.scale.x = reader.readF32();
				model.stationary_light.scale.y = reader.readF32();
				model.stationary_light.offset.x = reader.readF32();
				model.stationary_light.offset.y = reader.readF32();

				// Texture overrides per model (version >= 12)
				if (this->version >= 12) {
					if (this->version < 17) {
						model.texture_overrides.clear();
						MAPGEOTextureOverride ovr;
						ovr.index = 0;
						ovr.path = reader.readStringSized32();
						model.texture_overrides.push_back(ovr);
						model.texture_overrides_scale_offset.x = reader.readF32();
						model.texture_overrides_scale_offset.y = reader.readF32();
						model.texture_overrides_scale_offset.z = reader.readF32();
						model.texture_overrides_scale_offset.w = reader.readF32();
					} else {
						std::uint32_t texture_override_count = reader.readU32();
						model.texture_overrides.resize(texture_override_count);
						for (auto& texture_override : model.texture_overrides) {
							texture_override.index = reader.readU32();
							texture_override.path = reader.readStringSized32();
						}
						model.texture_overrides_scale_offset.x = reader.readF32();
						model.texture_overrides_scale_offset.y = reader.readF32();
						model.texture_overrides_scale_offset.z = reader.readF32();
						model.texture_overrides_scale_offset.w = reader.readF32();
					}
				}
			}
		}

		// Bucket grids and planar reflectors
		// Note: Some modded files may end here; we proceed if data remains.
		// Bucket grids
		std::uint32_t bucket_grid_count = (this->version >= 15) ? reader.readU32() : 1;
		this->bucket_grids.resize(bucket_grid_count);
		for (auto& bucket_grid : this->bucket_grids) {
			if (this->version >= 15) {
				bucket_grid.hash = reader.readU32();
			}
			bucket_grid.min_x = reader.readF32();
			bucket_grid.min_z = reader.readF32();
			bucket_grid.max_x = reader.readF32();
			bucket_grid.max_z = reader.readF32();
			bucket_grid.max_stickout_x = reader.readF32();
			bucket_grid.max_stickout_z = reader.readF32();
			bucket_grid.bucket_size_x = reader.readF32();
			bucket_grid.bucket_size_z = reader.readF32();
			std::uint16_t bucket_count = reader.readU16();
			bucket_grid.is_disabled = reader.readBool();
			bucket_grid.bucket_grid_flags = static_cast<MAPGEOBucketGridFlag>(reader.readU8());
			std::uint32_t bg_vertex_count = reader.readU32();
			std::uint32_t bg_index_count = reader.readU32();

			if (!bucket_grid.is_disabled) {
				// Vertices
				bucket_grid.vertices.resize(bg_vertex_count);
				for (std::uint32_t i = 0; i < bg_vertex_count; ++i) {
					bucket_grid.vertices[i].x = reader.readF32();
					bucket_grid.vertices[i].y = reader.readF32();
					bucket_grid.vertices[i].z = reader.readF32();
				}

				// Indices
				bucket_grid.indices.resize(bg_index_count);
				reader.readBuffered(bucket_grid.indices, bg_index_count * sizeof(std::uint16_t));

				// Buckets (flattened bucket_count x bucket_count)
				bucket_grid.buckets.resize(static_cast<std::size_t>(bucket_count) * static_cast<std::size_t>(bucket_count));
				for (std::size_t r = 0; r < bucket_count; ++r) {
					for (std::size_t c = 0; c < bucket_count; ++c) {
						MAPGEOBucket& bucket = bucket_grid.buckets[r * bucket_count + c];
						bucket.max_stickout_x = reader.readF32();
						bucket.max_stickout_z = reader.readF32();
						bucket.start_index = reader.readU32();
						bucket.base_vertex = reader.readU32();
						bucket.inside_face_count = reader.readU16();
						bucket.sticking_out_face_count = reader.readU16();
					}
				}

				// Face visibility flags
				if (static_cast<std::uint8_t>(bucket_grid.bucket_grid_flags) & static_cast<std::uint8_t>(MAPGEOBucketGridFlag::HasFaceVisibilityFlags)) {
					std::size_t face_layer_count = bg_index_count / 3;
					bucket_grid.face_layers.resize(face_layer_count);
					for (std::size_t i = 0; i < face_layer_count; ++i) {
						bucket_grid.face_layers[i] = static_cast<MAPGEOBucketGridFlag>(reader.readU8());
					}
				}
			}
		}

		// Planar reflectors (version >= 13)
		if (this->version >= 13) {
			std::uint32_t pr_count = reader.readU32();
			this->planar_reflectors.resize(pr_count);
			for (auto& planar_reflector : this->planar_reflectors) {
				planar_reflector.transform = reader.readMtx4();
				// plane: two vec3
				Container3<float> plane_min, plane_max;
				plane_min.x = reader.readF32(); plane_min.y = reader.readF32(); plane_min.z = reader.readF32();
				plane_max.x = reader.readF32(); plane_max.y = reader.readF32(); plane_max.z = reader.readF32();
				
				planar_reflector.plane.x = plane_min;
				planar_reflector.plane.y = plane_max;

				planar_reflector.normal.x = reader.readF32();
				planar_reflector.normal.y = reader.readF32();
				planar_reflector.normal.z = reader.readF32();
			}
		}
	}

	void MAPGEO::write(std::ostringstream& outp_file, std::uint32_t version_to_write, bool is_float16) {
		if (version_to_write != 17 && version_to_write != 13) {
			throw std::runtime_error(std::format("Writing MAPGEO version {} is not supported.", version_to_write));
		}
		this->version = version_to_write;
		BinaryWriter writer{ outp_file };

		struct VertexBufferInfo {
			MAPGEOLayer layer;
			std::vector<MAPGEOVertexElement> elements;
			const std::vector<MAPGEOVertex>* vertices;
		};

		struct IndexBufferInfo {
			MAPGEOLayer layer;
			const std::vector<std::uint16_t>* indices;
		};

		auto floatToHalf = [](float value) -> std::uint16_t {
			std::uint32_t bits = 0;
			std::memcpy(&bits, &value, sizeof(bits));

			std::uint32_t sign = (bits >> 16) & 0x8000u;
			std::uint32_t mantissa = bits & 0x007fffffu;
			int exp = static_cast<int>((bits >> 23) & 0xffu) - 127 + 15;

			if (exp <= 0) {
				if (exp < -10) {
					return static_cast<std::uint16_t>(sign);
				}
				mantissa = (mantissa | 0x00800000u) >> (1 - exp);
				return static_cast<std::uint16_t>(sign | ((mantissa + 0x1000u) >> 13));
			}

			if (exp >= 31) {
				return static_cast<std::uint16_t>(sign | 0x7c00u);
			}

			return static_cast<std::uint16_t>(sign | (static_cast<std::uint32_t>(exp) << 10) | ((mantissa + 0x1000u) >> 13));
		};

		auto inferFormatFromValue = [](const VertexDataVariant& value) -> MAPGEOVertexElementFormat {
			if (std::holds_alternative<float>(value)) {
				return MAPGEOVertexElementFormat::X_Float32;
			}
			if (std::holds_alternative<Container2<float>>(value)) {
				return MAPGEOVertexElementFormat::XY_Float32;
			}
			if (std::holds_alternative<Container3<float>>(value)) {
				return MAPGEOVertexElementFormat::XYZ_Float32;
			}
			if (std::holds_alternative<Container4<float>>(value)) {
				return MAPGEOVertexElementFormat::XYZW_Float32;
			}
			if (std::holds_alternative<Container4<std::uint8_t>>(value)) {
				return MAPGEOVertexElementFormat::BGRA_Packed8888;
			}
			if (std::holds_alternative<Container2<std::uint16_t>>(value)) {
				return MAPGEOVertexElementFormat::XY_Packed1616;
			}
			if (std::holds_alternative<Container3<std::uint16_t>>(value)) {
				return MAPGEOVertexElementFormat::XYZ_Packed161616;
			}
			if (std::holds_alternative<Container4<std::uint16_t>>(value)) {
				return MAPGEOVertexElementFormat::XYZW_Packed16161616;
			}

			throw std::runtime_error("Unable to infer vertex element format from value");
		};

		auto chooseFormatForElement = [&](MAPGEOVertexElementName name, const VertexDataVariant& value) {
			switch (name) {
				case MAPGEOVertexElementName::Position:
					return MAPGEOVertexElementFormat::XYZ_Float32;
				case MAPGEOVertexElementName::Normal:
					return is_float16 ? MAPGEOVertexElementFormat::XYZ_Packed161616 : MAPGEOVertexElementFormat::XYZ_Float32;
				case MAPGEOVertexElementName::PrimaryColor:
					return MAPGEOVertexElementFormat::BGRA_Packed8888;
				case MAPGEOVertexElementName::Texcoord0:
				case MAPGEOVertexElementName::Texcoord7:
					return is_float16 ? MAPGEOVertexElementFormat::XY_Packed1616 : MAPGEOVertexElementFormat::XY_Float32;
				case MAPGEOVertexElementName::Texcoord5:
					return MAPGEOVertexElementFormat::XYZ_Float32;
				default:
					return inferFormatFromValue(value);
			}
		};

		auto writeElementValue = [&](const MAPGEOVertexElement& element, const VertexDataVariant& value) {
			switch (element.format) {
				case MAPGEOVertexElementFormat::X_Float32: {
					if (const auto* v = std::get_if<float>(&value)) {
						writer.writeF32(*v);
						return;
					}
					break;
				}
				case MAPGEOVertexElementFormat::XY_Float32: {
					if (const auto* v = std::get_if<Container2<float>>(&value)) {
						writer.writeF32(v->x);
						writer.writeF32(v->y);
						return;
					}
					break;
				}
				case MAPGEOVertexElementFormat::XYZ_Float32: {
					if (const auto* v = std::get_if<Container3<float>>(&value)) {
						writer.writeF32(v->x);
						writer.writeF32(v->y);
						writer.writeF32(v->z);
						return;
					}
					break;
				}
				case MAPGEOVertexElementFormat::XYZW_Float32: {
					if (const auto* v = std::get_if<Container4<float>>(&value)) {
						writer.writeF32(v->x);
						writer.writeF32(v->y);
						writer.writeF32(v->z);
						writer.writeF32(v->w);
						return;
					}
					break;
				}
				case MAPGEOVertexElementFormat::BGRA_Packed8888:
				case MAPGEOVertexElementFormat::ZYXW_Packed8888:
				case MAPGEOVertexElementFormat::RGBA_Packed8888: {
					if (const auto* v = std::get_if<Container4<std::uint8_t>>(&value)) {
						writer.writeU8(v->x);
						writer.writeU8(v->y);
						writer.writeU8(v->z);
						writer.writeU8(v->w);
						return;
					}
					break;
				}
				case MAPGEOVertexElementFormat::XY_Packed1616: {
					if (const auto* v = std::get_if<Container2<std::uint16_t>>(&value)) {
						writer.writeU16(v->x);
						writer.writeU16(v->y);
						return;
					}
					if (const auto* v = std::get_if<Container2<float>>(&value)) {
						writer.writeU16(floatToHalf(v->x));
						writer.writeU16(floatToHalf(v->y));
						return;
					}
					break;
				}
				case MAPGEOVertexElementFormat::XYZ_Packed161616: {
					if (const auto* v = std::get_if<Container3<std::uint16_t>>(&value)) {
						writer.writeU16(v->x);
						writer.writeU16(v->y);
						writer.writeU16(v->z);
						writer.writeU16(0);
						return;
					}
					if (const auto* v = std::get_if<Container3<float>>(&value)) {
						writer.writeU16(floatToHalf(v->x));
						writer.writeU16(floatToHalf(v->y));
						writer.writeU16(floatToHalf(v->z));
						writer.writeU16(0);
						return;
					}
					break;
				}
				case MAPGEOVertexElementFormat::XYZW_Packed16161616: {
					if (const auto* v = std::get_if<Container4<std::uint16_t>>(&value)) {
						writer.writeU16(v->x);
						writer.writeU16(v->y);
						writer.writeU16(v->z);
						writer.writeU16(v->w);
						return;
					}
					if (const auto* v = std::get_if<Container4<float>>(&value)) {
						writer.writeU16(floatToHalf(v->x));
						writer.writeU16(floatToHalf(v->y));
						writer.writeU16(floatToHalf(v->z));
						writer.writeU16(floatToHalf(v->w));
						return;
					}
					break;
				}
				default:
					break;
			}

			throw std::runtime_error("Vertex element data does not match format");
		};

		this->vertex_descriptions.clear();
		std::vector<VertexBufferInfo> vertex_buffers;
		std::vector<IndexBufferInfo> index_buffers;
		vertex_buffers.reserve(this->models.size());
		index_buffers.reserve(this->models.size());

		const std::array<MAPGEOVertexElementName, 16> preferred_order = {
			MAPGEOVertexElementName::Position,
			MAPGEOVertexElementName::Normal,
			MAPGEOVertexElementName::PrimaryColor,
			MAPGEOVertexElementName::SecondaryColor,
			MAPGEOVertexElementName::BlendWeight,
			MAPGEOVertexElementName::BlendIndex,
			MAPGEOVertexElementName::FogCoordinate,
			MAPGEOVertexElementName::Texcoord0,
			MAPGEOVertexElementName::Texcoord1,
			MAPGEOVertexElementName::Texcoord2,
			MAPGEOVertexElementName::Texcoord3,
			MAPGEOVertexElementName::Texcoord4,
			MAPGEOVertexElementName::Texcoord5,
			MAPGEOVertexElementName::Texcoord6,
			MAPGEOVertexElementName::Texcoord7,
			MAPGEOVertexElementName::Tangent
		};

		for (auto& model : this->models) {
			if (model.vertices.empty()) {
				throw std::runtime_error("MAPGEO model has no vertices to write");
			}

			const auto& first_vertex = model.vertices.front();
			MAPGEOVertexDescription desc{};
			desc.usage = MAPGEOVertexUsage::Static;

			for (const auto name : preferred_order) {
				auto it = first_vertex.value.find(name);
				if (it == first_vertex.value.end()) {
					continue;
				}

				MAPGEOVertexElement element{};
				element.name = name;
				element.format = chooseFormatForElement(name, it->second);
				desc.elements.push_back(element);
			}

			if (desc.elements.empty()) {
				throw std::runtime_error("MAPGEO model has no vertex elements to write");
			}

			this->vertex_descriptions.push_back(desc);
			vertex_buffers.push_back(VertexBufferInfo{ model.layer, desc.elements, &model.vertices });
			index_buffers.push_back(IndexBufferInfo{ model.layer, &model.indices });

			bool has_position = false;
			Container3<float> bb_min;
			Container3<float> bb_max;
			bb_min.x = std::numeric_limits<float>::infinity();
			bb_min.y = std::numeric_limits<float>::infinity();
			bb_min.z = std::numeric_limits<float>::infinity();
			bb_max.x = -std::numeric_limits<float>::infinity();
			bb_max.y = -std::numeric_limits<float>::infinity();
			bb_max.z = -std::numeric_limits<float>::infinity();

			for (const auto& vertex : model.vertices) {
				for (const auto& element : desc.elements) {
					auto value_it = vertex.value.find(element.name);
					if (value_it == vertex.value.end()) {
						throw std::runtime_error("MAPGEO vertex missing expected element data");
					}

					if (element.name == MAPGEOVertexElementName::Position) {
						const auto* pos = std::get_if<Container3<float>>(&value_it->second);
						if (!pos) {
							throw std::runtime_error("MAPGEO position element must be XYZ float32");
						}
						has_position = true;
						bb_min.x = std::min(bb_min.x, pos->x);
						bb_min.y = std::min(bb_min.y, pos->y);
						bb_min.z = std::min(bb_min.z, pos->z);
						bb_max.x = std::max(bb_max.x, pos->x);
						bb_max.y = std::max(bb_max.y, pos->y);
						bb_max.z = std::max(bb_max.z, pos->z);
					}
				}
			}

			if (has_position) {
				model.bounding_box.x = bb_min;
				model.bounding_box.y = bb_max;
			}
		}

		writer.writeString("OEGM");
		writer.writeU32(this->version);

		if (this->version > 13) {
			writer.writeU32(static_cast<std::uint32_t>(this->texture_overrides.size()));
			for (const auto& texture_override : this->texture_overrides) {
				writer.writeU32(texture_override.index);
				writer.writeStringSized32(texture_override.path);
			}
		} else {
			if (this->texture_overrides.size() > 0) {
				writer.writeStringSized32(this->texture_overrides[0].path);
				writer.writeStringSized32(this->texture_overrides[1].path);
			} else {
				writer.writeU32(0);
				writer.writeU32(0);
			}
		}

		writer.writeU32(static_cast<std::uint32_t>(this->vertex_descriptions.size()));
		for (const auto& vertex_description : this->vertex_descriptions) {
			writer.writeU32(static_cast<std::uint32_t>(vertex_description.usage));
			const std::uint32_t element_count = static_cast<std::uint32_t>(vertex_description.elements.size());
			writer.writeU32(element_count);
			for (const auto& element : vertex_description.elements) {
				writer.writeU32(static_cast<std::uint32_t>(element.name));
				writer.writeU32(static_cast<std::uint32_t>(element.format));
			}
			const std::uint32_t fill_count = 15 - element_count;
			for (std::uint32_t i = 0; i < fill_count; ++i) {
				writer.writeU32(static_cast<std::uint32_t>(MAPGEOVertexElementName::Position));
				writer.writeU32(static_cast<std::uint32_t>(MAPGEOVertexElementFormat::XYZ_Float32));
			}
		}

		writer.writeU32(static_cast<std::uint32_t>(vertex_buffers.size()));
		for (const auto& buffer : vertex_buffers) {
			if (this->version >= 13) {
				writer.writeU8(static_cast<std::uint8_t>(buffer.layer));
			}

			std::uint32_t vertex_stride = 0;
			for (const auto& element : buffer.elements) {
				vertex_stride += getElementSize(element.format);
			}
			const std::uint32_t vertex_count = static_cast<std::uint32_t>(buffer.vertices->size());
			const std::uint32_t buffer_size = vertex_stride * vertex_count;
			writer.writeU32(buffer_size);

			for (const auto& vertex : *buffer.vertices) {
				for (const auto& element : buffer.elements) {
					auto value_it = vertex.value.find(element.name);
					if (value_it == vertex.value.end()) {
						throw std::runtime_error("MAPGEO vertex missing element data while writing");
					}
					writeElementValue(element, value_it->second);
				}
			}
		}

		writer.writeU32(static_cast<std::uint32_t>(index_buffers.size()));
		for (const auto& buffer : index_buffers) {
			if (this->version >= 13) {
				writer.writeU8(static_cast<std::uint8_t>(buffer.layer));
			}
			const std::uint32_t index_count = static_cast<std::uint32_t>(buffer.indices->size());
			writer.writeU32(index_count * sizeof(std::uint16_t));
			for (const auto index : *buffer.indices) {
				writer.writeU16(index);
			}
		}

		writer.writeU32(static_cast<std::uint32_t>(this->models.size()));
		for (std::uint32_t model_id = 0; model_id < this->models.size(); ++model_id) {
			auto& model = this->models[model_id];
			const std::uint32_t vertex_count = static_cast<std::uint32_t>(model.vertices.size());
			const std::uint32_t index_count = static_cast<std::uint32_t>(model.indices.size());

			writer.writeU32(vertex_count);
			writer.writeU32(1);
			writer.writeU32(model_id);
			writer.writeU32(model_id);
			writer.writeU32(index_count);
			writer.writeU32(model_id);

			if (this->version >= 13) {
				writer.writeU8(static_cast<std::uint8_t>(model.layer));
			}

			if (this->version >= 15) {
				writer.writeU32(model.bucket_grid_hash);
			}

			writer.writeU32(static_cast<std::uint32_t>(model.submeshes.size()));
			for (const auto& submesh : model.submeshes) {
				writer.writeU32(submesh.hash);
				writer.writeStringSized32(submesh.name);
				writer.writeU32(submesh.index_start);
				writer.writeU32(submesh.index_count);
				writer.writeU32(submesh.min_vertex);
				writer.writeU32(submesh.max_vertex);
			}

			if (this->version != 5) {
				writer.writeBool(model.disable_backface_culling);
			}

			writer.writeContainer<Container3, float, 3>(model.bounding_box.x);
			writer.writeContainer<Container3, float, 3>(model.bounding_box.y);
			writer.writeScalar(model.matrix);
			writer.writeU8(static_cast<std::uint8_t>(model.quality));

			if (this->version >= 7 && this->version <= 12) {
				writer.writeU8(static_cast<std::uint8_t>(model.layer));
			}

			if (this->version >= 11) {
				if (this->version < 14) {
					writer.writeU8(static_cast<std::uint8_t>(model.render));
				} else {
					writer.writeBool(model.is_bush);
					if (this->version < 16) {
						writer.writeU8(static_cast<std::uint8_t>(model.render));
					} else {
						writer.writeU16(static_cast<std::uint16_t>(model.render));
					}
				}
			}

			writer.writeStringSized32(model.baked_light.path);
			writer.writeF32(model.baked_light.scale.x);
			writer.writeF32(model.baked_light.scale.y);
			writer.writeF32(model.baked_light.offset.x);
			writer.writeF32(model.baked_light.offset.y);

			if (this->version >= 9) {
				writer.writeStringSized32(model.stationary_light.path);
				writer.writeF32(model.stationary_light.scale.x);
				writer.writeF32(model.stationary_light.scale.y);
				writer.writeF32(model.stationary_light.offset.x);
				writer.writeF32(model.stationary_light.offset.y);

				if (this->version >= 12) {
					if (this->version < 17) {
						const std::string path0 = model.texture_overrides.empty() ? std::string() : model.texture_overrides[0].path;
						writer.writeStringSized32(path0);
						writer.writeF32(model.texture_overrides_scale_offset.x);
						writer.writeF32(model.texture_overrides_scale_offset.y);
						writer.writeF32(model.texture_overrides_scale_offset.z);
						writer.writeF32(model.texture_overrides_scale_offset.w);
					} else {
						writer.writeU32(static_cast<std::uint32_t>(model.texture_overrides.size()));
						for (const auto& texture_override : model.texture_overrides) {
							writer.writeU32(texture_override.index);
							writer.writeStringSized32(texture_override.path);
						}
						writer.writeF32(model.texture_overrides_scale_offset.x);
						writer.writeF32(model.texture_overrides_scale_offset.y);
						writer.writeF32(model.texture_overrides_scale_offset.z);
						writer.writeF32(model.texture_overrides_scale_offset.w);
					}
				}
			}
		}

		const std::vector<MAPGEOBucketGrid>* bucket_grids_to_write = &this->bucket_grids;
		std::vector<MAPGEOBucketGrid> bucket_grid_fallback;
		if (this->version < 15 && this->bucket_grids.empty()) {
			MAPGEOBucketGrid dummy{};
			dummy.is_disabled = true;
			bucket_grid_fallback.push_back(dummy);
			bucket_grids_to_write = &bucket_grid_fallback;
		}

		if (this->version >= 15) {
			writer.writeU32(static_cast<std::uint32_t>(bucket_grids_to_write->size()));
		}

		for (const auto& bucket_grid : *bucket_grids_to_write) {
			if (this->version >= 15) {
				writer.writeU32(bucket_grid.hash);
			}
			writer.writeF32(bucket_grid.min_x);
			writer.writeF32(bucket_grid.min_z);
			writer.writeF32(bucket_grid.max_x);
			writer.writeF32(bucket_grid.max_z);
			writer.writeF32(bucket_grid.max_stickout_x);
			writer.writeF32(bucket_grid.max_stickout_z);
			writer.writeF32(bucket_grid.bucket_size_x);
			writer.writeF32(bucket_grid.bucket_size_z);

			std::uint16_t bucket_count = 0;
			if (!bucket_grid.buckets.empty()) {
				const double root = std::sqrt(static_cast<double>(bucket_grid.buckets.size()));
				const auto root_int = static_cast<std::size_t>(root + 0.5);
				if (root_int * root_int != bucket_grid.buckets.size()) {
					throw std::runtime_error("Bucket grid buckets must form a square grid");
				}
				bucket_count = static_cast<std::uint16_t>(root_int);
			}
			writer.writeU16(bucket_count);
			writer.writeBool(bucket_grid.is_disabled);
			writer.writeU8(static_cast<std::uint8_t>(bucket_grid.bucket_grid_flags));
			writer.writeU32(static_cast<std::uint32_t>(bucket_grid.vertices.size()));
			writer.writeU32(static_cast<std::uint32_t>(bucket_grid.indices.size()));

			if (!bucket_grid.is_disabled) {
				for (const auto& vertex : bucket_grid.vertices) {
					writer.writeF32(vertex.x);
					writer.writeF32(vertex.y);
					writer.writeF32(vertex.z);
				}
				for (const auto index : bucket_grid.indices) {
					writer.writeU16(index);
				}
				for (std::size_t r = 0; r < bucket_count; ++r) {
					for (std::size_t c = 0; c < bucket_count; ++c) {
						const MAPGEOBucket& bucket = bucket_grid.buckets[r * bucket_count + c];
						writer.writeF32(bucket.max_stickout_x);
						writer.writeF32(bucket.max_stickout_z);
						writer.writeU32(bucket.start_index);
						writer.writeU32(bucket.base_vertex);
						writer.writeU16(bucket.inside_face_count);
						writer.writeU16(bucket.sticking_out_face_count);
					}
				}
				if (static_cast<std::uint8_t>(bucket_grid.bucket_grid_flags) & static_cast<std::uint8_t>(MAPGEOBucketGridFlag::HasFaceVisibilityFlags)) {
					for (const auto& face_layer : bucket_grid.face_layers) {
						writer.writeU8(static_cast<std::uint8_t>(face_layer));
					}
				}
			}
		}

		if (this->version >= 13) {
			writer.writeU32(static_cast<std::uint32_t>(this->planar_reflectors.size()));
			for (const auto& planar_reflector : this->planar_reflectors) {
				writer.writeScalar(planar_reflector.transform);
				writer.writeF32(planar_reflector.plane.x.x);
				writer.writeF32(planar_reflector.plane.x.y);
				writer.writeF32(planar_reflector.plane.x.z);
				writer.writeF32(planar_reflector.plane.y.x);
				writer.writeF32(planar_reflector.plane.y.y);
				writer.writeF32(planar_reflector.plane.y.z);
				writer.writeF32(planar_reflector.normal.x);
				writer.writeF32(planar_reflector.normal.y);
				writer.writeF32(planar_reflector.normal.z);
			}
		}
	}
}