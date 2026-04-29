#include "mapgeo.hpp"
#include <format>
#include <array>
#include <algorithm>


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
				element.format = static_cast<MAPGEOVertexElementFormat>(unpacked_u32s[element_id * 2 + 1]);
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
			{
				Container3<float> bb_min;
				bb_min.x = reader.readF32(); bb_min.y = reader.readF32(); bb_min.z = reader.readF32();
				Container3<float> bb_max;
				bb_max.x = reader.readF32(); bb_max.y = reader.readF32(); bb_max.z = reader.readF32();
				model.bounding_box.x = bb_min;
				model.bounding_box.y = bb_max;
			}

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
}