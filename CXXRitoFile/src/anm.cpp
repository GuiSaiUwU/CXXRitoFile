#include "anm.hpp"
#include "hash.hpp"

namespace RitoFile {
	ANM::ANM(std::stringstream& inpt_file) : reader(inpt_file) {
		if (!inpt_file.good()) {
			throw std::runtime_error("Stream not good");
		}

		this->error_metrics = ANMErrorMetrics{
			ANMErrorMetric{0.0f, 0.0f},
			ANMErrorMetric{0.0f, 0.0f},
			ANMErrorMetric{0.0f, 0.0f}
		};
	}

	void ANM::read() {
		this->signature = reader.readString(8);
		this->version = reader.readU32();

		if (this->signature == "r3d2canm") {
			this->file_size = reader.readU32();
			this->format_token = reader.readU32();
			this->flags1 = reader.readU32();

			std::uint32_t track_count = reader.readU32();
			std::uint32_t frame_count = reader.readU32();

			reader.pad(4); // cache

			float max_time = reader.readF32();
			this->fps = reader.readF32();
			this->duration = max_time * this->fps + 1;

			/* Error metrics */
			this->error_metrics.rotate.margin = reader.readF32();
			this->error_metrics.rotate.discontinuity_threshold = reader.readF32();

			this->error_metrics.translate.margin = reader.readF32();
			this->error_metrics.translate.discontinuity_threshold = reader.readF32();

			this->error_metrics.scale.margin = reader.readF32();
			this->error_metrics.scale.discontinuity_threshold = reader.readF32();

			auto translate_min = reader.readContainer<Container3, float, 3>();
			auto translate_max = reader.readContainer<Container3, float, 3>();

			auto scale_min = reader.readContainer<Container3, float, 3>();
			auto scale_max = reader.readContainer<Container3, float, 3>();
			std::int32_t frames_offset = reader.readI32();
			reader.pad(4);
			std::int32_t joint_hashes_offset = reader.readI32();

			if (frames_offset <= 0) {
				throw std::runtime_error("ANM has no frames.");
			}
			if (joint_hashes_offset <= 0) {
				throw std::runtime_error("ANM has no joint hashes.");
			}

			// Read joint hashes
			reader.seek(joint_hashes_offset + 12);
			std::vector<std::uint32_t> joint_hashes;
			reader.readBuffered(joint_hashes, track_count * sizeof(std::uint32_t));

			// Create tracks
			this->tracks.resize(track_count);
			for (std::uint32_t track_idx = 0; track_idx < track_count; track_idx++) {
				ANMTrack& track = this->tracks.at(track_idx);
				track.joint_hash = joint_hashes.at(track_idx);
			}

			// Read frames
			reader.seek(frames_offset + 12);
			for (std::uint32_t frame_idx = 0; frame_idx < frame_count; frame_idx++) {
				std::uint16_t compressed_time = reader.readU16();
				std::uint16_t bits = reader.readU16();

				std::vector<std::uint8_t> compressed_transform;
				reader.readBuffered(compressed_transform, 6); // translation, rotation, scale

				std::uint32_t joint_hash = joint_hashes.at(bits & 16383);
				
				ANMTrack* found_track = nullptr;
				for (auto& track : this->tracks) {
					if (track.joint_hash == joint_hash) {
						found_track = &track;
						break;
					}
				}

				if (found_track == nullptr) [[unlikely]] {
					// This frame has wrong joint hash?
					continue;
				}

				// Parse pose
				float time = (((compressed_time / 65535.0f) * max_time) * this->fps);

				ANMPose* pose_ptr = nullptr;
				if (found_track->poses.contains(time)) {
					pose_ptr = &found_track->poses.at(time);
				}
				else {
					ANMPose new_pose;
					auto [insert_it, inserted] = found_track->poses.emplace(time, new_pose);
					pose_ptr = &insert_it->second;
				}

				// Decompress pose data
				std::uint8_t transform_type = (bits >> 14);

				if (transform_type == 0) {
					pose_ptr->rotate = ANMHelper::decompress_quaternion(compressed_transform);
				}
				else if (transform_type == 1) {
					pose_ptr->translate = ANMHelper::decompress_vector3(translate_min, translate_max, compressed_transform);
				}
				else if (transform_type == 2) {
					pose_ptr->scale = ANMHelper::decompress_vector3(scale_min, scale_max, compressed_transform);
				}
				else {
					throw std::runtime_error("Invalid transform type: " + std::to_string(transform_type));
				}
			}
		}
		else if (this->signature == "r3d2anmd") {
			if (this->version == 5) {
				// headers
				this->file_size = reader.readU32();
				this->format_token = reader.readU32();
				this->flags1 = reader.readU32();
				this->flags2 = reader.readU32();

				std::uint32_t track_count = reader.readU32();
				std::uint32_t frame_count = reader.readU32();

				this->fps = 1 / reader.readF32();
				this->duration = static_cast<float>(frame_count);

				// read offsets and calculate stuffs
				/* joint_hashes_offset, _, _, vecs_offset, quats_offset, frames_offset = bs.read_i32(6)*/
				std::int32_t joint_hashes_offset = reader.readI32();
				reader.pad(4 + 4);
				std::int32_t vecs_offset = reader.readI32();
				std::int32_t quats_offset = reader.readI32();
				std::int32_t frames_offset = reader.readI32();

				if (joint_hashes_offset <= 0) {
					throw std::runtime_error("ANM File does not contain joint hashes data.");
				}
				if (vecs_offset <= 0) {
					throw std::runtime_error("ANM File does not contain unique vectors data.");
				}
				if (quats_offset <= 0) {
					throw std::runtime_error("ANM File does not contain unique quaternions data.");
				}
				if (frames_offset <= 0) {
					throw std::runtime_error("ANM File does not contain frames data.");
				}

				std::uint32_t joint_hash_count = (frames_offset - joint_hashes_offset) / 4;
				std::uint32_t vec_count = (quats_offset - vecs_offset) / 12;
				std::uint32_t quat_count = (joint_hashes_offset - quats_offset) / 6;

				// Read joint hashes
				reader.seek(joint_hashes_offset + 12);
				std::vector<std::uint32_t> joint_hashes;
				reader.readBuffered(joint_hashes, joint_hash_count * sizeof(std::uint32_t));

				// Read unique vectors
				reader.seek(vecs_offset + 12);
				std::vector<Container3<float>> vec_bank;
				reader.readBuffered(vec_bank, vec_count * sizeof(Container3<float>));

				// Read unique quaternions
				reader.seek(quats_offset + 12);
				std::vector<Quaternion> quat_bank;
				quat_bank.reserve(quat_count);
				for (std::uint32_t quat_idx = 0; quat_idx < quat_count; quat_idx++) {
					std::vector<std::uint8_t> compressed_quat;
					reader.readBuffered(compressed_quat, 6);
					quat_bank.push_back(ANMHelper::decompress_quaternion(compressed_quat));
				}

				// Prepare tracks
				this->tracks.resize(track_count);
				for (std::uint32_t track_idx = 0; track_idx < track_count; track_idx++) {
					ANMTrack& track = this->tracks.at(track_idx);
					track.joint_hash = joint_hashes.at(track_idx);
				}

				// Read frames
				reader.seek(frames_offset + 12);
				for (float frame_idx = 0; frame_idx < frame_count; frame_idx++) {
					for (ANMTrack& track : this->tracks) {
						std::uint16_t translate_index = reader.readU16();
						std::uint16_t scale_index = reader.readU16();
						std::uint16_t rotate_index = reader.readU16();

						// Parse pose
						ANMPose pose;
						pose.translate = vec_bank.at(translate_index);
						pose.scale = vec_bank.at(scale_index);
						pose.rotate = quat_bank.at(rotate_index);

						track.poses[frame_idx] = pose;
					}
				}
			}
			else if (this->version == 4) {
				// Read headers
				this->file_size = reader.readU32();
				this->format_token = reader.readU32();
				this->flags1 = reader.readU32();
				this->flags2 = reader.readU32();

				std::uint32_t track_count = reader.readU32();
				std::uint32_t frame_count = reader.readU32();

				this->fps = 1 / reader.readF32() /*frame duration*/;
				this->duration = static_cast<float>(frame_count);

				//Read offsets & calculate stuffs
                /*_, _, _, vecs_offset, quats_offset, frames_offset = bs.read_i32(6)*/
				reader.pad(4 + 4 + 4);
				std::int32_t vecs_offset = reader.readI32();
				std::int32_t quats_offset = reader.readI32();
				std::int32_t frames_offset = reader.readI32();

				if (vecs_offset <= 0) {
					throw std::runtime_error("ANM File does not contain unique vectors data.");
				}
				if (quats_offset <= 0) {
					throw std::runtime_error("ANM File does not contain unique quaternions data.");
				}
				if (frames_offset <= 0) {
					throw std::runtime_error("ANM File does not contain frames data.");
				}

				std::uint32_t vec_count = (quats_offset - vecs_offset) / 12;
				std::uint32_t quat_count = (frames_offset - quats_offset) / 16;

				// Read unique vectors
				reader.seek(vecs_offset + 12);
				std::vector<Container3<float>> vec_bank;
				reader.readBuffered(vec_bank, vec_count * sizeof(Container3<float>));

				// Read unique quaternions
				reader.seek(quats_offset + 12);
				std::vector<Quaternion> quat_bank;
				reader.readBuffered(quat_bank, quat_count * sizeof(Quaternion));

				// Prepare tracks
				this->tracks.resize(track_count);

				// Read frames
				reader.seek(frames_offset + 12);
				for (std::uint32_t frame_idx = 0; frame_idx < frame_count; frame_idx++) {
					for (std::uint32_t track_idx = 0; track_idx < track_count; track_idx++) {
						std::uint32_t joint_hash = reader.readU32();
						std::uint16_t translate_index = reader.readU16();
						std::uint16_t scale_index = reader.readU16();
						std::uint16_t rotate_index = reader.readU16();

						reader.pad(2);

						// Find or create track for this joint hash
						ANMTrack* target_track = nullptr;

						// First, check if current track slot is empty or matches
						if (this->tracks[track_idx].joint_hash == 0 ||
							this->tracks[track_idx].joint_hash == joint_hash) {
							target_track = &this->tracks[track_idx];
							if (target_track->joint_hash == 0) {
								target_track->joint_hash = joint_hash;
							}
						}
						else {
							// Search for existing track with this joint hash
							for (auto& track : this->tracks) {
								if (track.joint_hash == joint_hash) {
									target_track = &track;
									break;
								}
							}

							// If no track found, use current slot
							if (target_track == nullptr) {
								target_track = &this->tracks[track_idx];
								target_track->joint_hash = joint_hash;
								target_track->poses.clear();
							}
						}

						// Parse pose
						ANMPose pose;
						pose.translate = vec_bank.at(translate_index);
						pose.scale = vec_bank.at(scale_index);
						pose.rotate = quat_bank.at(rotate_index);
						target_track->poses[static_cast<float>(target_track->poses.size())] = pose;
					}
				}
			}
			else if (version == 3) {
				// legacy
				//read headers
				reader.pad(4); // skl id
				std::uint32_t track_count = reader.readU32();
				std::uint32_t frame_count = reader.readU32();
				this->fps = 1.0f / reader.readU32();
				this->duration = static_cast<float>(frame_count);
				
				// Prepare tracks
				this->tracks.resize(track_count);
				for (ANMTrack& track : this->tracks) {
					track.joint_hash = elf(reader.readStringPadded(32));
					reader.pad(4); // flags

					// Read pose
					for (std::uint32_t frame_idx = 0; frame_idx < frame_count; frame_idx++) {
						ANMPose pose;
						pose.rotate = reader.readQuaternion();
						pose.translate = reader.readContainer<Container3, float, 3>();

						// legacy dont support scale
						pose.scale = Container3<float>{ 1.0f, 1.0f, 1.0f };

						track.poses[static_cast<float>(frame_idx)] = pose;
					}
				}
			}
		}
		else {
			throw std::runtime_error("Invalid ANM signature: " + this->signature);
		}
	}


	[[nodiscard]] Quaternion ANMHelper::decompress_quaternion(std::vector<std::uint8_t>& bytes)
	{
		if (bytes.size() < 6) [[unlikely]] {
			throw std::invalid_argument("Byte array must be at least 6 bytes long.");
		}
		std::uint64_t bits =
			(std::uint64_t{bytes[0]} << (0*8)) +
			(std::uint64_t{bytes[1]} << (1*8)) +
			(std::uint64_t{bytes[2]} << (2*8)) +
			(std::uint64_t{bytes[3]} << (3*8)) +
			(std::uint64_t{bytes[4]} << (4*8)) +
			(std::uint64_t{bytes[5]} << (5*8));

		std::uint8_t max_index = static_cast<std::uint8_t>((bits >> 45) & 3);

		float one_div_sqrt2 = 0.70710678118f;
		float sqrt2_div_32767 = 0.00004315969f;

		float a = static_cast<float>((bits >> 30) & 0x7FFF) * sqrt2_div_32767 - one_div_sqrt2;
		float b = static_cast<float>((bits >> 15) & 0x7FFF) * sqrt2_div_32767 - one_div_sqrt2;
		float c = static_cast<float>(bits & 0x7FFF) * sqrt2_div_32767 - one_div_sqrt2;
		float d = std::sqrt(std::max(0.0f, 1.0f - (a * a + b * b + c * c)));

		switch (max_index) {
		case 0: return Quaternion{ d, a, b, c };
		case 1: return Quaternion{ a, d, b, c };
		case 2: return Quaternion{ a, b, d, c };
		case 3: return Quaternion{ a, b, c, d };
		default: return Quaternion{ 1.0f, 0.0f, 0.0f, 0.0f }; // Should never happen
		}
	}

	[[nodiscard]] Container3<float> ANMHelper::decompress_vector3(Container3<float>& min, Container3<float>& max, std::vector<std::uint8_t>& bytes)
	{
		if (bytes.size() < 6) [[unlikely]] {
			throw std::invalid_argument("Byte array must be at least 6 bytes long.");
		}

		return Container3<float>(
			(max.x - min.x) / 65535.0f * (bytes[0] | bytes[1] << 8) + min.x,
			(max.y - min.y) / 65535.0f * (bytes[2] | bytes[3] << 8) + min.y,
			(max.z - min.z) / 65535.0f * (bytes[4] | bytes[5] << 8) + min.z
		);
	}

}