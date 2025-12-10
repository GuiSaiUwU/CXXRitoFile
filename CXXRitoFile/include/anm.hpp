#pragma once
#include "structs.hpp"
#include <vector>
#include "binary_stream.hpp"
#include <map>


namespace RitoFile {
	struct ANMErrorMetric {
		float margin, discontinuity_threshold;
	};

	struct ANMErrorMetrics {
		ANMErrorMetric translate;
		ANMErrorMetric rotate;
		ANMErrorMetric scale;
	};

	struct ANMPose {
		Container3<float> translate;
		Container3<float> scale;
		Quaternion rotate;
	};

	struct ANMTrack {
		std::uint32_t joint_hash;
		std::map<std::uint32_t, ANMPose> poses;
	};

	class ANM {
	public:
		/*
	    __slots__ = (
		    'signature', 'version', 'file_size', 'format_token', 'flags1', 'flags2',
			'duration', 'fps', 'error_metrics', 'tracks'
		)
		*/
		std::string signature;
		std::uint32_t version, file_size, format_token, flags1, flags2;
		float duration, fps;
		ANMErrorMetrics error_metrics;
		std::vector<ANMTrack> tracks;

		BinaryReader reader;

		ANM(std::stringstream& inpt_file);
		void read();
		void write(std::ostringstream& outp_file);
	};

	class ANMHelper {
	public:
		// bytes size must be 6
		static Quaternion decompress_quaternion(std::vector<std::uint8_t>& bytes);
		static Container3<float> decompress_vector3(Container3<float>& min, Container3<float>& max, std::vector<std::uint8_t>& bytes);

		
		static std::vector<uint8_t> compress_quaternion(const Quaternion& quat);
		
		static void interpolate_integer_frames(ANM& anm);
		static void build_frames(ANM& anm);
	};

}