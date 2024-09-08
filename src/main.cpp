#include <iostream>
#include "Lexer.h"
#include "Parser.h"

void printUsage(std::string exec) {
	std::string filename = exec.substr(exec.find_last_of("/\\") + 1);
	std::cout << "Usage: " << filename << " input_file [options]" << std::endl;
	std::cout << '\t' << "-j, --json filepath" << '\t' << "output ast to json in filepath" << std::endl;
	std::cout << '\t' << "-d, --dot filepath" << '\t' << "output ast to dot in filepath" << std::endl;
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cout << "Incorrect input!" << std::endl; printUsage(argv[0]);
		return 0;
	}

	int a = 1;
	std::shared_ptr<Stmt> ast;

    try {
		std::shared_ptr<Lexer> l = std::make_shared<Lexer>(argv[a++]);
		std::shared_ptr<Parser> p = std::make_shared<Parser>(l);
        ast = p->program();
    }
    catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return -1;
    }

	std::ofstream os;

	while (a < argc) {
		if (strcmp(argv[a], "-j") == 0 || strcmp(argv[a], "--json") == 0) {
		
			if (argv[a++] == nullptr) {
				std::cout << "Incorrect input!" << std::endl; printUsage(argv[0]);
				return 0;
			}

			// Write AST to json
			json j = ast->toJson();
			os.open(argv[a]);
			if (os.is_open()) os << j.dump();
			else std::cerr << "Can not open " << argv[a] << std::endl;
			os.close(); os.clear();
		}

		if (strcmp(argv[a], "-d") == 0 || strcmp(argv[a], "--dot") == 0) {

			if (argv[a++] == nullptr) {
				std::cout << "Incorrect input!" << std::endl; printUsage(argv[0]);
				return 0;
			}

			// Запись АСД в формате dot
			std::stringstream ss;
			ss << "digraph AST {" << std::endl;
			ss << '\t' << "node[fontname = \"helvetica\"]" << std::endl;
			ast->toDot(ss);
			ss << "}" << std::endl;

			os.open(argv[a]);
			if (os.is_open()) os << ss.str();
			else std::cerr << "Can not open " << argv[a] << std::endl;
			os.close(); os.clear();
		}

		a++;
	}

	return 0;
}