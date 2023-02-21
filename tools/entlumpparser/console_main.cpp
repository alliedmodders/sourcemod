#include <fstream>
#include <iostream>
#include <sstream>

#include "LumpManager.h"

int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cout << "Missing input file\n";
		return 0;
	}
	
	const char* filepath = argv[1];
	
	std::ifstream input(filepath, std::ios_base::binary);
	std::string data((std::istreambuf_iterator<char>(input)),  std::istreambuf_iterator<char>());
	
	EntityLumpManager lumpmgr;
	lumpmgr.Parse(data.c_str());
	
	std::cout << lumpmgr.Dump() << "\n";
}
