#include "bin.hpp"

namespace RitoFile {
	BIN::BIN(std::ifstream& inpt_file) : reader(inpt_file) {
		if (!inpt_file.good()) {
			throw std::runtime_error("Stream not good");
		}
	}

	void BIN::read() {

	}
	
	void BIN::write(std::ostringstream& outp_file) {
		throw std::runtime_error("Not implemented");
	}
}