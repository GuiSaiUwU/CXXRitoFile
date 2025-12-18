#include "mapgeo.hpp"

namespace RitoFile {
	MAPGEO::MAPGEO(std::stringstream& inpt_file)
		: reader(inpt_file) {
		if (!inpt_file.good()) {
			throw std::runtime_error("Input file stream is not good for reading MAPGEO.");
		}
	}

}